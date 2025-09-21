// stonehenge.c  —  太陽位置 + 巨石陣對準檢測（UTF-8 原始碼）
// 依賴 chinese.h 的輸入/輸出巨集
#include <math.h>
#include <stdio.h>
#include "chinese.h"

// --- 基本常數/巨集 ---
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
static inline double deg2rad(double d){ return d * (M_PI/180.0); }
static inline double rad2deg(double r){ return r * (180.0/M_PI); }
static inline double fmod360(double x){ double y=fmod(x,360.0); return (y<0)?y+360.0:y; }

// --- 1) 儒略日/世紀 ---
double julian_day(int y,int m,int D,int hh,int mm,int ss){
    if(m <= 2){ y -= 1; m += 12; }
    int A = y/100;
    int B = 2 - A + A/25;        // 格里曆修正
    double dayfrac = (hh + (mm + ss/60.0)/60.0)/24.0;
    long long iY = (long long)(365.25*(y+4716));
    long long iM = (long long)(30.6001*(m+1));
    return iY + iM + D + dayfrac + B - 1524.5;
}
double julian_century(double JD){ return (JD - 2451545.0)/36525.0; }

// --- 2) 太陽位置（近似 SPA-lite）---
// 參考：NOAA Solar Calculator 簡化版
void solar_position(double JD, double lat_deg, double lon_deg, double *alt_deg, double *az_deg){
    double T   = julian_century(JD);
    double L0  = fmod360(280.46646 + 36000.76983*T + 0.0003032*T*T); // 平黃經
    double M   = fmod360(357.52911 + 35999.05029*T - 0.0001537*T*T); // 平近點角
    double Mr  = deg2rad(M);

    double C   = (1.914602 - 0.004817*T - 0.000014*T*T)*sin(Mr)
               + (0.019993 - 0.000101*T)*sin(2*Mr)
               + 0.000289*sin(3*Mr);                                 // 心偏差
    double true_long = L0 + C;                                        // 真黃經
    double omega = 125.04 - 1934.136*T;                               // 進動
    double lambda = true_long - 0.00569 - 0.00478*sin(deg2rad(omega));// 視黃經

    double eps0 = 23.439291 - 0.0130042*T - 1.64e-7*T*T + 5.04e-7*T*T*T; // 平黃赤交角
    double eps  = eps0 + 0.00256*cos(deg2rad(omega));                 // 真黃赤交角

    // 赤經/赤緯
    double sin_lambda = sin(deg2rad(lambda));
    double cos_lambda = cos(deg2rad(lambda));
    double sin_eps    = sin(deg2rad(eps));
    double cos_eps    = cos(deg2rad(eps));

    double ra  = rad2deg(atan2(cos_eps*sin_lambda, cos_lambda));      // 赤經(度)
    double dec = rad2deg(asin(sin_eps*sin_lambda));                   // 赤緯(度)
    if(ra < 0) ra += 360.0;

    // 格林威治平恒星時（近似）
    double T0 = (JD - 2451545.0)/36525.0;
    double theta = fmod360(280.46061837 + 360.98564736629*(JD-2451545.0)
                           + 0.000387933*T0*T0 - T0*T0*T0/38710000.0);

    // 地方恒星時
    double LST = fmod360(theta + lon_deg);

    // 時角（度）
    double H = fmod360(LST - ra);
    if(H > 180.0) H -= 360.0; // 換到 (-180,180]

    // 地平座標
    double lat = deg2rad(lat_deg);
    double Hr  = deg2rad(H);
    double decr= deg2rad(dec);

    double sin_alt = sin(lat)*sin(decr) + cos(lat)*cos(decr)*cos(Hr);
    double alt = asin(sin_alt);

    double cos_az = (sin(decr) - sin(lat)*sin(alt)) / (cos(lat)*cos(alt));
    if(cos_az >  1) cos_az = 1;
    if(cos_az < -1) cos_az = -1;

    double az = acos(cos_az); // 0..π（相對南方）
    // 轉為「由北順時針」的方位角（常見定義）：北=0° 東=90° 南=180° 西=270°
    if(sin(Hr) > 0) az = 2*M_PI - az;

    *alt_deg = rad2deg(alt);
    *az_deg  = rad2deg(az);
}

// --- 2.5) 大氣折射修正（標準 Bennett 公式）---
double refract_deg(double alt_deg){
    // Bennett (1982) 標準公式：R = 1 / tan(h + 7.31/(h + 4.4)) 弧分
    // 假設標準大氣壓和溫度
    double h = alt_deg;
    if (h < -1.0) h = -1.0;  // 避免極端值
    double R_arcmin = 1.0 / tan((h + 7.31/(h + 4.4)) * M_PI/180.0);
    return alt_deg + R_arcmin/60.0;
}

// --- 3) 互動：輸入時間/位置，輸出太陽高度與方位 ---
static void 輸入年月日時區(int *Y,int *M,int *D,int *h,int *m,int *s,double *tz){
    輸出字串("請輸入年份(例如 2025)："); *Y = 輸入整數();
    輸出字串("請輸入月份(1-12)：");       *M = 輸入整數();
    輸出字串("請輸入日期(1-31)：");       *D = 輸入整數();
    輸出字串("請輸入小時(0-23)：");       *h = 輸入整數();
    輸出字串("請輸入分鐘(0-59)：");       *m = 輸入整數();
    輸出字串("請輸入秒(0-59)：");         *s = 輸入整數();
    輸出字串("請輸入時區(例如 英國夏令時≈+1、冬令時=0；台北=+8)：");
    double tz_in = 0.0; // 用 scanf 讀 double
#ifdef _MSC_VER
    scanf_s("%lf", &tz_in);
#else
    scanf("%lf", &tz_in);
#endif
    tz_in = 1.0;  // 硬編碼測試
    *tz = tz_in;
}

static void 印出結果(double alt,double az){
    輸出格式("太陽高度角：%.3f°\n", alt);
    輸出格式("太陽方位角：%.3f°（北=0°，東=90°，南=180°，西=270°）\n", az);
}

// --- 4) 巨石陣對準檢測 ---
// 位置：緯度 51.1789°N，經度 -1.8262°（西經為負）；夏至日清晨日出方向約在 ~49°（視差/地平折射略有差異）
// 我們容許±2°作為「接近對準」的簡易門檻。
static const double STONEHENGE_LAT = 51.1789;
static const double STONEHENGE_LON = 1.8262;
static const double SOLSTICE_RISE_AZIMUTH = 49.0;
static const double ALIGN_TOL = 2.0;

static void 檢查巨石陣對準(double az){
    double diff = fabs(az - SOLSTICE_RISE_AZIMUTH);
    if(diff > 180.0) diff = 360.0 - diff;
    if(diff <= ALIGN_TOL){
        輸出字串("判定：接近巨石陣夏至日出對準（±2°）\n");
    }else{
        輸出格式("判定：未對準（與 %.1f° 相差 %.2f°）\n", SOLSTICE_RISE_AZIMUTH, diff);
    }
}

// --- 4.5) 自動掃描日出時刻 ---
static void 掃描日出(int Y, int M, int D, double tz_hours){
    double best_abs = 1e9, best_JD=0, best_alt=0, best_az=0;
    // 掃 03:30 ~ 05:30，每30秒
    for(int sec=0; sec<=2*3600; sec+=30){
        int hh = 3 + sec/3600;
        int mm = (sec/60) % 60;
        int ss = sec % 60;
        double JD_local = julian_day(Y, M, D, hh, mm, ss);
        double JD_utc   = JD_local - tz_hours/24.0;

        double alt_g, az;
        solar_position(JD_utc, STONEHENGE_LAT, STONEHENGE_LON, &alt_g, &az);
        double horizon_elev_deg = 0.3;  // 地平線仰角
        double alt_app = refract_deg(alt_g) - horizon_elev_deg;

        double v = fabs(alt_app);
        if(v < best_abs){ best_abs=v; best_JD=JD_local; best_alt=alt_app; best_az=az; }
    }
    輸出格式("最接近日出時刻(當地)：JD=%.5f，高度=%.3f°，方位=%.3f°\n", best_JD, best_alt, best_az);
}

// --- 4.6) 超快掃描器：掃描指定日期在當地時區的日出窗口（含折射+地平線）---
void 掃描日出窗口(int Y,int M,int D, double tz_hours, int start_h, int end_h,
               double horizon_deg, int step_s) {
    double best_abs = 1e9, best_alt=0, best_az=0;
    int    best_h=0,best_m=0,best_s=0;

    // 起點JD：當地 00:00
    double JD0_local = julian_day(Y,M,D,0,0,0);

    for (int t = start_h*3600; t <= end_h*3600; t += step_s){
        int hh =  t/3600;
        int mm = (t/60)%60;
        int ss =  t%60;

        double JD_local = JD0_local + (hh + mm/60.0 + ss/3600.0)/24.0;
        double JD_utc   = JD_local - tz_hours/24.0;

        double alt_g, az;
        solar_position(JD_utc, STONEHENGE_LAT, STONEHENGE_LON, &alt_g, &az);

        // 折射 + 地平線仰角
        double alt = refract_deg(alt_g) - horizon_deg;

        double v = fabs(alt);
        if (v < best_abs){
            best_abs = v; best_alt=alt; best_az=az;
            best_h=hh; best_m=mm; best_s=ss;
        }
    }

    輸出格式("最接近地平線：%02d:%02d:%02d  高度=%.3f°  方位=%.3f°\n",
             best_h, best_m, best_s, best_alt, best_az);
}

// --- 5) 封裝成主功能 ---
void 巨石陣太陽模擬(void){
    輸出字串("\n=== 巨石陣太陽模擬 ===\n");
    輸出字串("預設地點：英國巨石陣（緯 51.1789 N, 經 -1.8262）\n");

    int Y,M,D,h,mi,s; double tz;
    輸入年月日時區(&Y,&M,&D,&h,&mi,&s,&tz);
    輸出格式("tz = %.1f\n", tz);

    // 時區轉 UTC → 計算 JD
    int    uh = h - (int)tz;    // 只做簡化處理：tz 以小時為單位
    double frac = (tz - (int)tz);
    int    um = mi - (int)round(frac*60.0);
    // 正規化分鐘/小時（簡易，足夠互動）
    while(um < 0){ um += 60; uh -= 1; }
    while(um >= 60){ um -= 60; uh += 1; }

    double JD_utc = julian_day(Y, M, D, uh, um, s);

    double alt_geom, az;
    solar_position(JD_utc, STONEHENGE_LAT, STONEHENGE_LON, &alt_geom, &az);
    double horizon_elev_deg = 0.3;  // 地平線仰角（可改）
    double alt = refract_deg(alt_geom) - horizon_elev_deg;
    印出結果(alt, az);

    // 日出時高度角接近 0；但我們僅示範方位對準的概念
    檢查巨石陣對準(az);

    // 額外：掃描當日最接近日出的時刻（舊版）
    掃描日出(Y, M, D, tz);

    // 新版超快掃描器：日出窗口 00:00-08:00，步長10秒，地平線仰角0°
    輸出字串("--- 超快掃描器測試 ---\n");
    掃描日出窗口(Y, M, D, tz, 0, 8, 0.0, 10);

    輸出字串("-----------------------------\n\n");
}