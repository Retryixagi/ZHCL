// kukulcan.c  —  羽蛇神神殿（El Castillo）蛇影近似模擬
// 依賴：你的 chinese.h、solar_position()、refract_deg()（可直接複用 stonehenge.c 的版本）
#include <math.h>
#include <stdio.h>
#include "chinese.h"

// 從 stonehenge.c 複用的函數聲明
double julian_day(int y,int m,int D,int hh,int mm,int ss);
void solar_position(double JD, double lat_deg, double lon_deg, double *alt_deg, double *az_deg);
double refract_deg(double alt_deg);

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
static inline double deg2rad(double d){ return d * (M_PI/180.0); }
static inline double rad2deg(double r){ return r * (180.0/M_PI); }
static inline double clamp01(double x){ return x<0?0:(x>1?1:x); }
static inline double gaussian(double x, double mu, double sigma){
    double z = (x-mu)/sigma; return exp(-0.5*z*z);
}

// ========== 地點（可調） ==========
static const double KUK_LAT = 20.684;   // Chichén Itzá 約略緯度
static const double KUK_LON = -88.567;  // 約略經度（西經為負）

// ========== 幾何近似參數（可在程式中輸入/校正） ==========
// 金字塔面向：理想為正北；蛇影集中在「北面」靠西側的階梯/欄杆（balustrade）。
// 近似條件：
//  1) 低仰角太陽（接近日落）：10° ~ 20° 最合適（可改）
//  2) 太陽方位靠西；為了投射到北面西側欄杆，方位落在 255°~285° 區間較佳（可改）
//  3) 階梯「鋸齒」— 用步高/步寬比近似出需要的影長節距；我們用可調參數讓你校正。
typedef struct {
    double target_alt_deg;   // 理想仰角（度）
    double alt_sigma;        // 仰角高斯寬度
    double target_az_deg;    // 理想方位角（度，北=0，東=90，西=270）
    double az_sigma;         // 方位角高斯寬度
    double horizon_deg;      // 地平線仰角（地形/建築遮擋），預設 0.5°
    double terrace_rise_m;   // 單層步高（m），粗估 0.5 m
    double terrace_run_m;    // 單層步寬（m），粗估 0.9 m
    double balustrade_yaw;   // 欄杆軸線相對正北的偏角（度）；西北階梯可設 ~ -45（可微調）
} KukulcanParams;

static KukulcanParams default_params(void){
    KukulcanParams p;
    p.target_alt_deg = 14.0;    // 近日落的低仰角
    p.alt_sigma      = 4.0;
    p.target_az_deg  = 265.0;   // 西南偏西（微調到 265°）
    p.az_sigma       = 7.0;
    p.horizon_deg    = 0.5;
    p.terrace_rise_m = 0.5;
    p.terrace_run_m  = 0.9;
    p.balustrade_yaw = -45.0;   // 西北角階梯（北面靠西）沿著 -45° 方向延伸
    return p;
}

// ========== 可見度指數（0..1） ==========
// 由三個因素組合：方位吻合、仰角吻合、鋸齒節距與投影節距匹配。
// 投影節距近似：對一個步高h、步寬w 的鋸齒，在太陽仰角 alt 時，沿著面法線方向的影長 ≈ h / tan(alt)
// 而沿著欄杆軸向的投影節距，也取決於太陽與欄杆的夾角；我們用 cos(Δ方位) 做一個一階近似縮放。
static double serpent_visibility(double alt_deg, double az_deg, const KukulcanParams *pp)
{
    // 1) 大氣折射/地平線校正已在外面做，這裡假設 alt_deg 是「視高度 – 地平線仰角」後的值
    // 2) 高斯權重（方位 & 仰角）
    double w_az  = gaussian(az_deg,  pp->target_az_deg,  pp->az_sigma);
    double w_alt = gaussian(alt_deg, pp->target_alt_deg, pp->alt_sigma);

    // 3) 鋸齒節距匹配（簡式）
    // 期望：在欄杆方向上，明暗三角的投影節距 ≈ 某個可見比例（我們用步寬/步高比來設理想仰角）
    // 這裡把理想仰角反推為 arctan(h/run_eff)，run_eff 取步寬乘上 |cos(太陽方位-欄杆方位)|
    double dAz = fabs(az_deg - (pp->balustrade_yaw < 0 ? (360.0+pp->balustrade_yaw) : pp->balustrade_yaw));
    if(dAz > 180.0) dAz = 360.0 - dAz;
    double run_eff = pp->terrace_run_m * fabs(cos(deg2rad(dAz)));
    // 避免 run_eff 太小導致數值暴走
    if (run_eff < 0.2) run_eff = 0.2;

    double alt_ideal = rad2deg(atan2(pp->terrace_rise_m, run_eff));
    double w_pitch   = gaussian(alt_deg, alt_ideal, 10.0); // 放寬到 10°

    // 綜合分數（可權重）
    double score = pow(w_az, 0.6) * pow(w_alt, 0.6) * pow(w_pitch, 0.8);
    return clamp01(score);
}

// ========== 互動：單點計算 ==========
void 羽蛇神單次模擬(void)
{
    輸出字串("\n=== 羽蛇神神殿（El Castillo）蛇影模擬 ===\n");
    int Y,M,D,h,mi,s;
    double tz;
    輸出字串("請輸入年份：");  Y = 輸入整數();
    輸出字串("請輸入月份：");  M = 輸入整數();
    輸出字串("請輸入日期：");  D = 輸入整數();
    輸出字串("請輸入小時：");  h = 輸入整數();
    輸出字串("請輸入分鐘：");  mi= 輸入整數();
    輸出字串("請輸入秒：");    s = 輸入整數();
    輸出字串("請輸入時區（例：當地多為 -5；台北 +8）：");
#ifdef _MSC_VER
    scanf_s("%lf", &tz);
#else
    scanf("%lf", &tz);
#endif

    double JD_local = julian_day(Y,M,D,h,mi,s);
    double JD_utc   = JD_local - tz/24.0;

    double alt_g, az;
    solar_position(JD_utc, KUK_LAT, KUK_LON, &alt_g, &az);

    KukulcanParams P = default_params();
    // 視高度（折射）- 地平線仰角
    double alt = refract_deg(alt_g) - P.horizon_deg;

    double vis = serpent_visibility(alt, az, &P);

    輸出格式("太陽幾何高度：%.3f°；視高度-地平線：%.3f°；方位：%.3f°\n", alt_g, alt, az);
    輸出格式("蛇影可見度指數（0~1）：%.3f\n", vis);
    if (vis > 0.6) 輸出字串("判定：強可見（接近最佳條件）\n");
    else if (vis > 0.35) 輸出字串("判定：可見（條件尚可）\n");
    else 輸出字串("判定：不易辨識（偏離最佳條件）\n");
    輸出字串("-----------------------------\n\n");
}

// ========== 掃描（找最佳蛇影時刻） ==========
// 給定日期與時區，在 [start_h, end_h]（當地時間）每 step_s 秒掃描一次，找分數最高的時刻。
void 掃描羽蛇神最佳時刻(int Y,int M,int D, double tz_hours,
                     int start_h, int end_h, int step_s)
{
    KukulcanParams P = default_params();

    double best_score = -1.0;
    int    bh=0,bm=0,bs=0;
    double b_alt=0,b_az=0,b_altg=0;

    double JD0_local = julian_day(Y,M,D,0,0,0);
    for (int t = start_h*3600; t <= end_h*3600; t += step_s){
        int hh =  t/3600;
        int mm = (t/60)%60;
        int ss =  t%60;

        double JD_local = JD0_local + (hh + mm/60.0 + ss/3600.0)/24.0;
        double JD_utc   = JD_local - tz_hours/24.0;

        double alt_g, az;
        solar_position(JD_utc, KUK_LAT, KUK_LON, &alt_g, &az);

        double alt = refract_deg(alt_g) - P.horizon_deg;
        double sc  = serpent_visibility(alt, az, &P);

        if (sc > best_score){
            best_score = sc; bh=hh; bm=mm; bs=ss; b_alt=alt; b_az=az; b_altg=alt_g;
        }
    }

    輸出格式("最佳條件（當地時間）%02d:%02d:%02d  分數=%.3f  視高=%.2f°  方位=%.2f°（幾何高=%.2f°）\n",
             bh,bm,bs,best_score,b_alt,b_az,b_altg);
    輸出字串("（春分/秋分附近，建議掃 15:30~18:30；步長先 30 秒再 1 秒細化）\n");
}