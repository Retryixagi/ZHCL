// zhcc_full.cpp — 使用 chinese.h 的「完整 CLI 工具」（非教學樣本）
// 需求：你的 chinese.h（提供 輸出字串/輸出整數/輸出小數/輸出格式/初始化中文環境/用時間當種子/當前秒 等）
//
// 編譯：
//   Windows (MSVC):  cl /std:c++17 /EHsc /utf-8 zhcc_full.cpp /Fe:zhcc_full.exe
//   Linux/macOS:     g++ -std=c++17 zhcc_full.cpp -o zhcc_full
//
// 例：
//   zhcc_full math --circle 5
//   zhcc_full math --pow 2 10
//   zhcc_full math --sin 1.0471975512        # 弧度（≈60°）
//   zhcc_full sun  --lat 51.1789 --lon -1.8262 --tz 1 --iso 2025-06-21T04:52:00
//   zhcc_full sun  --lat 51.1789 --lon -1.8262 --tz 0 --ymd 2025 6 21 --hms 4 52 0

#include "chinese.h"

#include <cmath>
#include <cstring>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <algorithm>

// ---------------------------- 基本工具 ----------------------------
static inline double deg2rad(double d){ return d * M_PI / 180.0; }
static inline double rad2deg(double r){ return r * 180.0 / M_PI; }

// 安全取參
static bool getArg(int argc, char** argv, const char* key, std::string& val){
    for(int i=1;i<argc;i++){
        if(std::strcmp(argv[i], key)==0 && i+1<argc){
            val = argv[i+1];
            return true;
        }
    }
    return false;
}
static bool hasFlag(int argc, char** argv, const char* key){
    for(int i=1;i<argc;i++) if(std::strcmp(argv[i], key)==0) return true;
    return false;
}

static void print_usage(){
    輸出字串(
        "用法:\n"
        "  zhcc_full math --circle R\n"
        "  zhcc_full math --pow X N\n"
        "  zhcc_full math --sin RAD | --cos RAD | --sqrt X\n"
        "  zhcc_full sun  --lat LAT --lon LON --tz TZ --iso YYYY-MM-DDThh:mm:ss\n"
        "  zhcc_full sun  --lat LAT --lon LON --tz TZ --ymd Y M D --hms h m s\n"
    );
}

// ---------------------------- 數學子系統 ----------------------------
static int cmd_math(int argc, char** argv){
    if (hasFlag(argc, argv, "--circle")){
        std::string sr; if(!getArg(argc, argv, "--circle", sr)){ 輸出字串("缺少半徑\n"); return 1; }
        double r = std::strtod(sr.c_str(), nullptr);
        double area = M_PI * r * r;
        輸出字串("圓面積："); 輸出小數(area);
        return 0;
    }
    if (hasFlag(argc, argv, "--pow")){
        std::string sx, sn; 
        if(!getArg(argc, argv, "--pow", sx) || argc< (int)(std::find(argv, argv+argc, std::string("--pow"))-argv)+3){
            // 第二個參數 N 讀取
        }
        // 手動抓兩個值
        for(int i=1;i<argc;i++){
            if(std::strcmp(argv[i],"--pow")==0){
                if(i+1<argc) sx = argv[i+1];
                if(i+2<argc) sn = argv[i+2];
                break;
            }
        }
        if(sx.empty()||sn.empty()){ 輸出字串("缺少底數或指數\n"); return 1; }
        double x = std::strtod(sx.c_str(), nullptr);
        double n = std::strtod(sn.c_str(), nullptr);
        double y = std::pow(x, n);
        輸出字串("冪次結果："); 輸出小數(y);
        return 0;
    }
    if (hasFlag(argc, argv, "--sin")){
        std::string srad; if(!getArg(argc, argv, "--sin", srad)){ 輸出字串("缺少弧度\n"); return 1; }
        double r = std::strtod(srad.c_str(), nullptr);
        輸出字串("sin(rad)："); 輸出小數(std::sin(r)); return 0;
    }
    if (hasFlag(argc, argv, "--cos")){
        std::string srad; if(!getArg(argc, argv, "--cos", srad)){ 輸出字串("缺少弧度\n"); return 1; }
        double r = std::strtod(srad.c_str(), nullptr);
        輸出字串("cos(rad)："); 輸出小數(std::cos(r)); return 0;
    }
    if (hasFlag(argc, argv, "--sqrt")){
        std::string sx; if(!getArg(argc, argv, "--sqrt", sx)){ 輸出字串("缺少數值\n"); return 1; }
        double x = std::strtod(sx.c_str(), nullptr);
        輸出字串("sqrt："); 輸出小數(std::sqrt(x)); return 0;
    }
    輸出字串("math 子命令需要指定 --circle / --pow / --sin / --cos / --sqrt\n");
    return 1;
}

// ---------------------------- 太陽位置計算（簡化 SPA） ----------------------------
// 參考 NOAA Solar Calculator 的常見近似：
// 步驟：
//   1) 計算儒略日 JD 與 J2000 以來日數 n
//   2) 中太陽平近點角 g，黃經 q，黃道經度 L
//   3) 地軸傾角 e，太陽赤經 RA、赤緯 dec
//   4) 方位計算需當地真太陽時（LST），時角 H
//   5) 高度/方位：
//        sin(alt) = sin(lat)*sin(dec) + cos(lat)*cos(dec)*cos(H)
//        az = atan2( -sin(H), tan(dec)*cos(lat) - sin(lat)*cos(H) ) 轉為 [0,360)
// 注意：這是足夠工程模擬用的近似，非天文觀測級全精度。
struct DateTime {
    int Y,M,D,h,m,s;
    double tz;   // 時區（小時）
};

static bool parse_iso(const std::string& iso, DateTime& dt){
    // YYYY-MM-DDThh:mm:ss
    if (iso.size() < 19) return false;
    dt.Y = std::atoi(iso.substr(0,4).c_str());
    dt.M = std::atoi(iso.substr(5,2).c_str());
    dt.D = std::atoi(iso.substr(8,2).c_str());
    dt.h = std::atoi(iso.substr(11,2).c_str());
    dt.m = std::atoi(iso.substr(14,2).c_str());
    dt.s = std::atoi(iso.substr(17,2).c_str());
    return true;
}

static double julian_day(int Y,int M,int D,int h,int m,int s, double tz){
    // 轉成 UTC 時間
    double hour = h + m/60.0 + s/3600.0 - tz;
    // 演算法：NOAA 常用
    int a = (14 - M)/12;
    int y = Y + 4800 - a;
    int m2 = M + 12*a - 3;
    int JDN = D + (153*m2 + 2)/5 + 365*y + y/4 - y/100 + y/400 - 32045;
    double JD = JDN + (hour - 12.0)/24.0;
    return JD;
}

static void sun_position(double lat_deg, double lon_deg, const DateTime& dt, double& altitude_deg, double& azimuth_deg){
    double JD = julian_day(dt.Y,dt.M,dt.D,dt.h,dt.m,dt.s, dt.tz);
    double n = JD - 2451545.0;             // days since J2000.0
    double L = fmod(280.460 + 0.9856474*n, 360.0); if(L<0) L+=360.0;   // mean longitude
    double g = fmod(357.528 + 0.9856003*n, 360.0); if(g<0) g+=360.0;   // mean anomaly
    double g_rad = deg2rad(g);

    // ecliptic longitude (approx)
    double lambda = L + 1.915*std::sin(g_rad) + 0.020*std::sin(2*g_rad);
    double lambda_rad = deg2rad(lambda);

    // obliquity of the ecliptic
    double e = 23.439 - 0.0000004*n;
    double e_rad = deg2rad(e);

    // RA/Dec
    double sinlambda = std::sin(lambda_rad);
    double coslambda = std::cos(lambda_rad);
    double tanlambda = std::tan(lambda_rad);
    double sin_e = std::sin(e_rad);
    double cos_e = std::cos(e_rad);

    double RA = std::atan2( cos_e * sinlambda, coslambda );     // in rad
    double dec = std::asin( sin_e * sinlambda );                 // in rad

    // GMST (Greenwich mean sidereal time) in degrees
    double GMST = fmod(280.46061837 + 360.98564736629 * n, 360.0);
    if(GMST<0) GMST += 360.0;

    // Local sidereal time
    double LST = GMST + lon_deg;
    LST = fmod(LST, 360.0);
    if(LST<0) LST += 360.0;

    // Hour angle H = LST - RA(deg)
    double RAdeg = rad2deg(RA);
    double H = LST - RAdeg;
    H = fmod(H+540.0, 360.0)-180.0; // [-180,180)

    // Convert to radians
    double lat = deg2rad(lat_deg);
    double Hrad = deg2rad(H);

    // Altitude
    double alt = std::asin( std::sin(lat)*std::sin(dec) + std::cos(lat)*std::cos(dec)*std::cos(Hrad) );
    // Azimuth (from North, eastward)
    double az = std::atan2( -std::sin(Hrad),
                            std::tan(dec)*std::cos(lat) - std::sin(lat)*std::cos(Hrad) );
    double az_deg = fmod(rad2deg(az)+360.0, 360.0);

    altitude_deg = rad2deg(alt);
    azimuth_deg  = az_deg;
}

static int cmd_sun(int argc, char** argv){
    std::string slat, slon, stz, siso;
    std::string sY,sM,sD,sh,sm,ss;

    if(!getArg(argc, argv, "--lat", slat) || !getArg(argc, argv, "--lon", slon) || !getArg(argc, argv, "--tz", stz)){
        輸出字串("缺少 --lat/--lon/--tz\n"); return 1;
    }
    DateTime dt{}; dt.tz = std::strtod(stz.c_str(), nullptr);
    if(getArg(argc, argv, "--iso", siso)){
        if(!parse_iso(siso, dt)){ 輸出字串("無法解析 --iso\n"); return 1; }
    }else{
        if(!getArg(argc, argv, "--ymd", sY) || !getArg(argc, argv, "--hms", sh)){
            // 逐一抓
            getArg(argc, argv, "--ymd", sY); getArg(argc, argv, "--hms", sh);
        }
        // 容錯：允許分別提供
        getArg(argc, argv, "--Y", sY); getArg(argc, argv, "--M", sM); getArg(argc, argv, "--D", sD);
        getArg(argc, argv, "--h", sh); getArg(argc, argv, "--m", sm); getArg(argc, argv, "--s", ss);

        // 若用 --ymd / --hms 的形式，就在 argv 內挨著放數字（Y M D）（h m s）
        // 這裡保守直接逐項讀
        if(!sY.empty() && !sM.empty() && !sD.empty()){
            dt.Y = std::atoi(sY.c_str());
            dt.M = std::atoi(sM.c_str());
            dt.D = std::atoi(sD.c_str());
        } else {
            // 嘗試從 argv 序列中解析
            // 尋找 --ymd 後三個整數
            for(int i=1;i<argc;i++){
                if(std::strcmp(argv[i],"--ymd")==0 && i+3<argc){
                    dt.Y = std::atoi(argv[i+1]);
                    dt.M = std::atoi(argv[i+2]);
                    dt.D = std::atoi(argv[i+3]);
                }
                if(std::strcmp(argv[i],"--hms")==0 && i+3<argc){
                    dt.h = std::atoi(argv[i+1]);
                    dt.m = std::atoi(argv[i+2]);
                    dt.s = std::atoi(argv[i+3]);
                }
            }
        }
        if(dt.Y==0 || dt.M==0 || dt.D==0){ 輸出字串("請提供有效日期 (--iso 或 --ymd Y M D)\n"); return 1; }
        if(sh.size()) dt.h = std::atoi(sh.c_str());
        if(sm.size()) dt.m = std::atoi(sm.c_str());
        if(ss.size()) dt.s = std::atoi(ss.c_str());
    }

    double lat = std::strtod(slat.c_str(), nullptr);
    double lon = std::strtod(slon.c_str(), nullptr);

    double alt_deg=0.0, az_deg=0.0;
    sun_position(lat, lon, dt, alt_deg, az_deg);

    輸出字串("太陽高度角(deg)："); 輸出小數(alt_deg);
    輸出字串("太陽方位角(deg，北=0、東=90)："); 輸出小數(az_deg);
    return 0;
}

// ---------------------------- 入口 ----------------------------
int main(int argc, char** argv){
    初始化中文環境();
    用時間當種子();

    if (argc < 2){
        print_usage();
        return 1;
    }

    std::string sub = argv[1];
    if (sub == std::string("math")) return cmd_math(argc-1, argv+1);
    if (sub == std::string("sun"))  return cmd_sun(argc-1, argv+1);

    print_usage();
    return 1;
}
