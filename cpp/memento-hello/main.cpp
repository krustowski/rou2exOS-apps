#include "ui/platform/impl/UIImpl.h"
#include "ui/platform/PlatformWindow.h"
#include "ui/platform/PlatformKey.h"
#include "ui/platform/PlatformDrawingContext.h"
#include "ui/platform/PlatformBitmap.h"
#include "ui/platform/PlatformColor.h"
#include "ui/platform/PlatformFont.h"

using namespace Memento;

#ifdef MEMENTO_BACKEND_R2
extern "C" {
    long set_video_mode(unsigned char mode);

    // ScListTasks (0x2F) — forward-declare to avoid FBInfo_T conflict with R2_LL.h
    struct TaskInfo_T {
        unsigned char id, mode, status, _pad;
        unsigned char name[16];
    } __attribute__((packed));
    long list_tasks(TaskInfo_T* buf, unsigned char max);

    // ScNetStatus (0x38)
    struct NetStatus_T {
        unsigned char  mac[6];
        unsigned char  ip[4];
        unsigned char  drv_active;
        unsigned char  n_ports;
        unsigned short ports[16];
    } __attribute__((packed));
    long get_net_status(NetStatus_T* ns);

    // ScListMounts (0x2C)
    struct MountInfo_T {
        unsigned char path[32];
        unsigned char path_len;
        unsigned char fs_type;
    } __attribute__((packed));
    long list_mounts(MountInfo_T* buf);

    // ScListDirPath (0x2D)
    struct VfsDirEntry_T {
        unsigned char name[32];
        unsigned char name_len;
        unsigned char is_dir;
        unsigned int  size;
    } __attribute__((packed));
    long list_dir_path(const unsigned char* path, VfsDirEntry_T* buf);

    // ScReadFile (0x20)
    long read_file(const unsigned char* name, unsigned char* buf);

    // ScChdir (0x2E)
    long chdir(const unsigned char* path);

    // Heap checkpoint/restore for the Desktop navigation loop
    unsigned long r2_heap_checkpoint();
    void          r2_heap_restore(unsigned long cp);

    long run_elf(const unsigned char* name, const unsigned char* args, unsigned char* pid);
    void sleep_ms(unsigned long long ms);

    // ScSysInfo (0x01) — system config read/write
    struct SysInfo_T {
        unsigned char  system_name[32];
        unsigned char  system_user[32];
        unsigned char  system_path[32];
        unsigned char  system_version[8];
        unsigned int   system_path_cluster;
        unsigned int   system_uptime;
        unsigned char  ip_addr[4];
    } __attribute__((packed));
    long read_sysinfo(SysInfo_T* sysinfo);
}
#endif

// 
// Window 1 — Hello
// 

class HelloWindow {
public:
    bool wantsNext = false;

    static void onEvent(void* instance, struct PlatformWindowInterfaceInputEvent* data) {
        reinterpret_cast<HelloWindow*>(instance)->onEvent_(data);
    }

    void SetWindow(PlatformWindow* w) { wnd = w; }

private:
    PlatformWindow* wnd  = nullptr;
    PlatformColor*  bg   = nullptr;
    PlatformColor*  fg   = nullptr;
    PlatformFont*   font = nullptr;

    void onEvent_(struct PlatformWindowInterfaceInputEvent* data) {
        if (data->type == PlatformWindowInputEventType::OnPaint) {
            OnPaint(data->Data.OnPaint.ctx, data->Data.OnPaint.target);
        } else if (data->type == PlatformWindowInputEventType::OnKeyEvent) {
            auto* key = data->Data.OnKeyEvent.key;
            if (key->isEscape) wnd->Close();
            if (key->isEnter)  { wantsNext = true; wnd->Close(); }
        }
    }

    void OnPaint(PlatformDrawingContext* dc, PlatformBitmap* target) {
        if (!bg)   bg   = dc->CreateColor(0xFF1A1A2E, nullptr, nullptr);
        if (!fg)   fg   = dc->CreateColor(0xFFE0E0FF, nullptr, nullptr);
        if (!font) font = dc->CreateFont(16, nullptr, false, false, false, nullptr, nullptr);
        if (!bg || !fg || !font) return;

        Coord w = target->GetWidth();
        Coord h = target->GetHeight();

        target->FillRect(0, 0, w, h, bg, false);

        PlatformDrawTextOptions opts{};
        opts.font            = font;
        opts.foreground      = fg;
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign   = PlatformAlign::Middle;

        target->DrawText(0, h / 4,      w, 32, "Hello r2!",       &opts, false);
        target->DrawText(0, h / 2 - 2,  w, 18, "Enter  -  login", &opts, false);
        target->DrawText(0, h / 2 + 18, w, 18, "ESC  -  quit",    &opts, false);
    }
};

// Wallpaper painted first; every window then draws on top of it.
// 2-colour VGA blit: R+G+B > 300 → white (palette 15), else → dark blue (1).
// Canvas: 256×160 px.  At DPI=86: W≈285 Coord, H≈178 Coord.
// Trunk is drawn bright (white) in the sky and dark (silhouette) on the beach.
static void drawWallpaper(PlatformDrawingContext* dc, PlatformBitmap* target) {
    static PlatformColor* lit = nullptr;  // lum > 300 → white
    static PlatformColor* dk  = nullptr;  // lum ≤ 300 → dark blue

    if (!lit) lit = dc->CreateColor(0xFFD0D0F8, nullptr, nullptr);
    if (!dk)  dk  = dc->CreateColor(0xFF050510, nullptr, nullptr);
    if (!lit) return;

    const Coord W = target->GetWidth();
    const Coord H = target->GetHeight();

    // ── Stars ────────────────────────────────────────────────────────────────
    static const Coord sx[] = { 20, 55, 95, 150, 35, 80, 190, 245 };
    static const Coord sy[] = { 12,  8, 20,  10, 35, 28,   8,  22 };
    for (int i = 0; i < 8; i++)
        target->FillRect(sx[i], sy[i], 2, 2, lit, false);

    // Moon — top-right
    target->FillRect(W - 30, 12, 9, 6, lit, false);

    // ── Sea horizon line ─────────────────────────────────────────────────────
    target->FillRect(0, H - 55, W, 1, lit, false);

    // ── Small waves left of island ───────────────────────────────────────────
    target->FillRect(10,  H - 52, 22, 2, lit, false);
    target->FillRect(45,  H - 50, 18, 2, lit, false);
    target->FillRect(80,  H - 53, 24, 2, lit, false);

    // ── Sandy beach — bright oval, right-aligned ─────────────────────────────
    target->FillRect(180, H - 52, W - 183, 4, lit, false);  // top
    target->FillRect(170, H - 48, W - 173, 7, lit, false);  // upper body
    target->FillRect(165, H - 41, W - 168, 9, lit, false);  // widest
    target->FillRect(170, H - 32, W - 173, 7, lit, false);  // lower body
    target->FillRect(180, H - 25, W - 183, 4, lit, false);  // bottom edge

    // ── Palm trunk ───────────────────────────────────────────────────────────
    // Sky zone (bright against dark sky): trunk top down to horizon
    target->FillRect(258, H - 108, 3, 56, lit, false);
    // Beach zone (dark silhouette on bright sand): horizon down to trunk base
    target->FillRect(258, H -  52, 3, 23, dk,  false);

    // ── Palm fronds (bright against dark sky) ────────────────────────────────
    // Left droop — 6 steps going left-and-down from crown
    target->FillRect(250, H - 108, 5, 3, lit, false);
    target->FillRect(242, H - 105, 5, 3, lit, false);
    target->FillRect(233, H - 101, 5, 3, lit, false);
    target->FillRect(223, H -  97, 5, 3, lit, false);
    target->FillRect(213, H -  93, 5, 3, lit, false);
    target->FillRect(203, H -  89, 5, 3, lit, false);

    // Right droop — 6 steps going right-and-down (clips off canvas edge, that's fine)
    target->FillRect(261, H - 108, 5, 3, lit, false);
    target->FillRect(269, H - 105, 5, 3, lit, false);
    target->FillRect(278, H - 101, 5, 3, lit, false);
    target->FillRect(288, H -  97, 5, 3, lit, false);
    target->FillRect(298, H -  93, 5, 3, lit, false);
    target->FillRect(308, H -  89, 5, 3, lit, false);

    // Upper-left frond — rises left from crown
    target->FillRect(250, H - 111, 5, 3, lit, false);
    target->FillRect(242, H - 114, 5, 3, lit, false);
    target->FillRect(233, H - 117, 5, 3, lit, false);
    target->FillRect(224, H - 119, 5, 3, lit, false);

    // Upper-right frond — rises right from crown (clips are OK)
    target->FillRect(262, H - 111, 5, 3, lit, false);
    target->FillRect(270, H - 114, 5, 3, lit, false);
    target->FillRect(279, H - 117, 5, 3, lit, false);
    target->FillRect(288, H - 119, 5, 3, lit, false);

    // Top frond — straight up from crown
    target->FillRect(255, H - 112, 4, 4, lit, false);
    target->FillRect(254, H - 116, 4, 4, lit, false);
    target->FillRect(253, H - 120, 4, 4, lit, false);
}

//
// Window 2 — Login dialog (username + password)
//

class LoginWindow {
public:
    static void onEvent(void* instance, struct PlatformWindowInterfaceInputEvent* data) {
        reinterpret_cast<LoginWindow*>(instance)->onEvent_(data);
    }

    void SetWindow(PlatformWindow* w) { wnd = w; }

    bool wantsDesktop = false;

private:
    PlatformWindow* wnd   = nullptr;
    PlatformColor*  dark  = nullptr;
    PlatformColor*  light = nullptr;
    PlatformFont*   font  = nullptr;
    int             focus = 0;  // 0=login field, 1=password field, 2=OK, 3=Cancel

    static const int MAX_LEN = 63;
    char loginBuf[64] = {};
    char passBuf[64]  = {};
    int  loginLen     = 0;
    int  passLen      = 0;

    void onEvent_(struct PlatformWindowInterfaceInputEvent* data) {
        if (data->type == PlatformWindowInputEventType::OnPaint) {
            OnPaint(data->Data.OnPaint.ctx, data->Data.OnPaint.target);
            return;
        }
        if (data->type != PlatformWindowInputEventType::OnKeyEvent) return;
        auto* key = data->Data.OnKeyEvent.key;
        if (!key->isKeyDown) return;

        if (key->isEscape) { wnd->Close(); return; }

        // Text input — explicit branches to avoid reference-to-ternary aliasing issues
        if (key->isChar) {
            if (focus == 0 && loginLen < MAX_LEN) {
                loginBuf[loginLen++] = (char)key->theChar;
                loginBuf[loginLen]   = 0;
                wnd->Repaint();
            } else if (focus == 1 && passLen < MAX_LEN) {
                passBuf[passLen++] = (char)key->theChar;
                passBuf[passLen]   = 0;
                wnd->Repaint();
            }
            return;
        }
        if (key->isBackspace) {
            if      (focus == 0 && loginLen > 0) { loginBuf[--loginLen] = 0; wnd->Repaint(); }
            else if (focus == 1 && passLen  > 0) { passBuf[--passLen]   = 0; wnd->Repaint(); }
            return;
        }

        // Navigation (Tab removed — it generates isTab before isKeyDown is checked)
        if (key->isArrowLeft || key->isArrowRight) {
            if (focus >= 2) { focus = (focus == 2) ? 3 : 2; wnd->Repaint(); }
            return;
        }
        if (key->isArrowDown) {
            focus = (focus + 1) % 4; wnd->Repaint(); return;
        }
        if (key->isArrowUp) {
            focus = (focus + 3) % 4; wnd->Repaint(); return;
        }
        if (key->isEnter) {
            if      (focus == 0) { focus = 1; wnd->Repaint(); }
            else if (focus == 1) { focus = 2; wnd->Repaint(); }
            else if (focus == 2) { wantsDesktop = true; wnd->Close(); }  // OK
            else                 { wnd->Close(); }                         // Cancel
        }
    }

    void DrawInputField(PlatformBitmap* target, Coord bx, Coord by, Coord bw, Coord bh,
                        const char* buf, int len, bool focused, bool isPassword) {
        char display[66] = {};
        int i = 0;

        for (; i < len; i++) display[i] = isPassword ? '*' : buf[i];

        if (focused) display[i++] = '_';
        display[i] = 0;
        PlatformDrawTextOptions opts{};
        opts.font            = font;
        opts.horizontalAlign = PlatformAlign::Begin;
        opts.verticalAlign   = PlatformAlign::Middle;

        if (focused) {
            target->FillRect(bx, by, bw, bh, dark, false);
            opts.foreground = light;
        } else {
            target->FillRect(bx, by, bw, bh, dark, false);
            target->FillRect(bx + 1, by + 1, bw - 2, bh - 2, light, false);
            opts.foreground = dark;
        }

        target->DrawText(bx + 3, by, bw - 6, bh, (const mchar*)display, &opts, false);
    }

    void DrawButton(PlatformBitmap* target, Coord bx, Coord by, Coord bw, Coord bh,
                    const mchar* label, bool focused) {
        PlatformDrawTextOptions opts{};
        opts.font            = font;
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign   = PlatformAlign::Middle;

        if (focused) {
            target->FillRect(bx, by, bw, bh, dark, false);
            opts.foreground = light;
        } else {
            target->FillRect(bx, by, bw, bh, dark, false);
            target->FillRect(bx + 1, by + 1, bw - 2, bh - 2, light, false);
            opts.foreground = dark;
        }

        target->DrawText(bx, by, bw, bh, label, &opts, false);
    }

    void OnPaint(PlatformDrawingContext* dc, PlatformBitmap* target) {
        if (!dark)  dark  = dc->CreateColor(0xFF0A0A20, nullptr, nullptr);
        if (!light) light = dc->CreateColor(0xFFE0E0FF, nullptr, nullptr);
        if (!font)  font  = dc->CreateFont(12, nullptr, false, false, false, nullptr, nullptr);
        if (!dark || !light || !font) return;

        Coord W = target->GetWidth();
        Coord H = target->GetHeight();

        target->FillRect(0, 0, W, H, dark, false);
        drawWallpaper(dc, target);

        // Taskbar
        target->FillRect(0, H - 14, W, 1,  dark,  false);
        target->FillRect(0, H - 13, W, 13, light, false);

        // Dialog: left-aligned, 185 Coord wide  (x=40..225, inner x=42..223)
        target->FillRect(40, 52, 185, 96, dark,  false);
        target->FillRect(42, 54, 181, 92, light, false);

        // Title bar separator at y=68, close button
        target->FillRect(42,  68, 181,  1, dark, false);
        target->FillRect(211, 57,  10,  8, dark, false);

        PlatformDrawTextOptions opts{};
        opts.font            = font;
        opts.foreground      = dark;
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign   = PlatformAlign::Middle;

        target->DrawText(42, 54, 165, 14, "Login",         &opts, false);
        target->DrawText(0, H - 13, W, 13, "Login  -  r2", &opts, false);

        // Row labels (left-aligned)
        opts.horizontalAlign = PlatformAlign::Begin;
        target->DrawText(50, 74, 58, 14, "Login:",    &opts, false);
        target->DrawText(50, 93, 58, 14, "Password:", &opts, false);

        // Input fields — right edge at inner right (223) minus 2px padding
        DrawInputField(target, 110, 74, 109, 14, loginBuf, loginLen, focus == 0, false);
        DrawInputField(target, 110, 93, 109, 14, passBuf,  passLen,  focus == 1, true);

        // Buttons
        DrawButton(target,  80, 118, 44, 18, "OK",     focus == 2);
        DrawButton(target, 140, 118, 60, 18, "Cancel", focus == 3);
    }
};

// 
// Window 4 — Task Manager  (live data via ScListTasks 0x2F)
// 

class TasksWindow {
public:
    static void onEvent(void* instance, struct PlatformWindowInterfaceInputEvent* data) {
        reinterpret_cast<TasksWindow*>(instance)->onEvent_(data);
    }
    void SetWindow(PlatformWindow* w) { wnd = w; }

private:
    PlatformWindow* wnd    = nullptr;
    PlatformColor*  dark   = nullptr;
    PlatformColor*  light  = nullptr;
    PlatformFont*   font   = nullptr;
    int             sel    = 0;
    int             nLive  = 0;  // last known task count; key handlers use it

    static const char* statusStr(unsigned char s) {
        if (s == 0) return "Ready";
        if (s == 1) return "Running";
        if (s == 2) return "Idle";
        if (s == 3) return "Blocked";
        if (s == 4) return "Crashed";
        if (s == 5) return "Dead";
        return "?";
    }
    static const char* modeStr(unsigned char m) { return m ? "User" : "Kernel"; }

    static void pidStr(unsigned char n, char* out) {
        if (n >= 100) { out[0]='0'+n/100; out[1]='0'+(n/10)%10; out[2]='0'+n%10; out[3]=0; }
        else if (n >= 10) { out[0]='0'+n/10; out[1]='0'+n%10; out[2]=0; }
        else { out[0]='0'+n; out[1]=0; }
    }

    void onEvent_(struct PlatformWindowInterfaceInputEvent* data) {
        if (data->type == PlatformWindowInputEventType::OnPaint) {
            OnPaint(data->Data.OnPaint.ctx, data->Data.OnPaint.target);
            return;
        }

        if (data->type != PlatformWindowInputEventType::OnKeyEvent) return;

        auto* key = data->Data.OnKeyEvent.key;

        if (!key->isKeyDown) return;
        if (key->isEscape) { wnd->Close(); return; }
        if (key->isArrowUp)   { if (sel > 0)      { sel--; wnd->Repaint(); } return; }
        if (key->isArrowDown) { if (sel < nLive)   { sel++; wnd->Repaint(); } return; }
        if (key->isEnter && sel == nLive) { wnd->Close(); }
    }

    void OnPaint(PlatformDrawingContext* dc, PlatformBitmap* target) {
        if (!target) return;
        if (!dark)  dark  = dc->CreateColor(0xFF0A0A20, nullptr, nullptr);
        if (!light) light = dc->CreateColor(0xFFE0E0FF, nullptr, nullptr);
        if (!font)  font  = dc->CreateFont(12, nullptr, false, false, false, nullptr, nullptr);
        if (!dark || !light || !font) return;

        // Fetch live task list (max 10, 20 bytes each = 200 bytes on stack)
        TaskInfo_T buf[10];
        int n = (int)list_tasks(buf, 10);
        if (n < 0) n = 0;
        nLive = n;
        if (sel > nLive) sel = nLive;

        Coord W = target->GetWidth();
        Coord H = target->GetHeight();

        target->FillRect(0, 0, W, H, dark, false);
        drawWallpaper(dc, target);
        target->FillRect(0, H - 14, W,  1, dark,  false);
        target->FillRect(0, H - 13, W, 13, light, false);

        // Window chrome — tall enough for 10 rows + header + back button
        // Outer y=8..182, inner y=10..180
        target->FillRect(5,  8,  310, 175, dark,  false);
        target->FillRect(7,  10, 306, 171, light, false);
        target->FillRect(7,  24, 306,   1, dark,  false);  // title separator

        PlatformDrawTextOptions opts{};
        opts.font            = font;
        opts.foreground      = dark;
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign   = PlatformAlign::Middle;

        target->DrawText(7,  10, 280, 14, "Task Manager",   &opts, false);
        target->DrawText(0, H - 13, W, 13, "Tasks  -  r2", &opts, false);

        // Column headers  (x: PID=10 Name=44 Mode=198 Status=248)
        opts.horizontalAlign = PlatformAlign::Begin;
        target->DrawText(10,  26, 30,  12, "PID",    &opts, false);
        target->DrawText(44,  26, 150, 12, "Name",   &opts, false);
        target->DrawText(198, 26, 46,  12, "Mode",   &opts, false);
        target->DrawText(248, 26, 58,  12, "Status", &opts, false);
        target->FillRect(7, 38, 306, 1, dark, false);

        // Task rows — 12 px each, starting at y=40
        for (int i = 0; i < n; i++) {
            Coord ry = 40 + i * 12;
            if (sel == i) {
                target->FillRect(8, ry, 304, 11, dark, false);
                opts.foreground = light;
            } else {
                opts.foreground = dark;
            }
            char pidbuf[4];  pidStr(buf[i].id, pidbuf);
            char namebuf[17];
            for (int j = 0; j < 16; j++) namebuf[j] = (char)buf[i].name[j];

            namebuf[16] = 0;

            opts.horizontalAlign = PlatformAlign::Begin;
            target->DrawText(10,  ry, 30,  11, (const mchar*)pidbuf,              &opts, false);
            target->DrawText(44,  ry, 150, 11, (const mchar*)namebuf,             &opts, false);
            target->DrawText(198, ry, 46,  11, (const mchar*)modeStr(buf[i].mode),&opts, false);
            target->DrawText(248, ry, 58,  11, (const mchar*)statusStr(buf[i].status), &opts, false);
        }

        // Separator + Back button
        target->FillRect(7, 163, 306, 1, dark, false);
        bool backFocused = (sel == nLive);
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign   = PlatformAlign::Middle;

        if (backFocused) {
            target->FillRect(120, 166, 80, 13, dark, false);
            opts.foreground = light;
        } else {
            target->FillRect(120, 166, 80, 13, dark, false);
            target->FillRect(121, 167, 78, 11, light, false);
            opts.foreground = dark;
        }

        target->DrawText(120, 166, 80, 13, "Back", &opts, false);
    }
};

// 
// Window — Network Status  (ScNetStatus 0x38)
// Shows IP, MAC, driver state and bound TCP port registry.
// 

class NetWindow {
public:
    static void onEvent(void* instance, struct PlatformWindowInterfaceInputEvent* data) {
        reinterpret_cast<NetWindow*>(instance)->onEvent_(data);
    }
    void SetWindow(PlatformWindow* w) { wnd = w; }

private:
    PlatformWindow* wnd   = nullptr;
    PlatformColor*  dark  = nullptr;
    PlatformColor*  light = nullptr;
    PlatformFont*   font  = nullptr;

    static char hexNibble(unsigned char n) { return n < 10 ? '0'+n : 'a'+(n-10); }

    static void byteToStr(unsigned char b, char* out) {
        if (b >= 100) { out[0]='0'+b/100; out[1]='0'+(b/10)%10; out[2]='0'+b%10; out[3]=0; }
        else if (b >= 10) { out[0]='0'+b/10; out[1]='0'+b%10; out[2]=0; }
        else { out[0]='0'+b; out[1]=0; }
    }

    static void u16ToStr(unsigned short n, char* out) {
        if (!n) { out[0]='0'; out[1]=0; return; }
        char t[6]; int i=0;
        while(n) { t[i++]='0'+n%10; n/=10; }
        for(int j=0;j<i;j++) out[j]=t[i-1-j]; out[i]=0;
    }

    // "a.b.c.d\0" — caller supplies buf[16]
    static void ipToStr(const unsigned char ip[4], char* buf) {
        int pos = 0;
        for (int i = 0; i < 4; i++) {
            char tmp[4]; byteToStr(ip[i], tmp);
            for (int j = 0; tmp[j]; j++) buf[pos++] = tmp[j];
            if (i < 3) buf[pos++] = '.';
        }
        buf[pos] = 0;
    }

    // "aa:bb:cc:dd:ee:ff\0" — caller supplies buf[18]
    static void macToStr(const unsigned char mac[6], char* buf) {
        for (int i = 0; i < 6; i++) {
            buf[i*3]   = hexNibble(mac[i] >> 4);
            buf[i*3+1] = hexNibble(mac[i] & 0xF);
            buf[i*3+2] = (i < 5) ? ':' : 0;
        }
        buf[17] = 0;
    }

    void onEvent_(struct PlatformWindowInterfaceInputEvent* data) {
        if (data->type == PlatformWindowInputEventType::OnPaint) {
            OnPaint(data->Data.OnPaint.ctx, data->Data.OnPaint.target);
            return;
        }
        if (data->type != PlatformWindowInputEventType::OnKeyEvent) return;
        auto* key = data->Data.OnKeyEvent.key;
        if (!key->isKeyDown) return;
        if (key->isEscape || key->isEnter) wnd->Close();
    }

    void OnPaint(PlatformDrawingContext* dc, PlatformBitmap* target) {
        if (!target) return;
        if (!dark)  dark  = dc->CreateColor(0xFF0A0A20, nullptr, nullptr);
        if (!light) light = dc->CreateColor(0xFFE0E0FF, nullptr, nullptr);
        if (!font)  font  = dc->CreateFont(12, nullptr, false, false, false, nullptr, nullptr);
        if (!dark || !light || !font) return;

        NetStatus_T ns{};
        get_net_status(&ns);
        SysInfo_T si{};
        read_sysinfo(&si);

        Coord W = target->GetWidth(), H = target->GetHeight();
        target->FillRect(0,   0,   W,   H,  dark,  false);
        drawWallpaper(dc, target);
        target->FillRect(0, H-14,  W,   1,  dark,  false);
        target->FillRect(0, H-13,  W,  13,  light, false);
        target->FillRect(5,   8, 310, 175,  dark,  false);
        target->FillRect(7,  10, 306, 171,  light, false);
        target->FillRect(7,  24, 306,   1,  dark,  false);  // title sep

        PlatformDrawTextOptions opts{};
        opts.font            = font;
        opts.foreground      = dark;
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign   = PlatformAlign::Middle;
        target->DrawText(7,    10, 280, 14, "Network",          &opts, false);
        target->DrawText(0,  H-13,   W, 13, "Net  -  r2",      &opts, false);

        // Section: Interface
        opts.horizontalAlign = PlatformAlign::Begin;

        // IP row — from sysinfo (set by ETH driver via ScSysInfo 0x02)
        char ipbuf[16]; ipToStr(si.ip_addr, ipbuf);
        target->DrawText(10, 28, 36, 12, "IP:",  &opts, false);
        target->DrawText(50, 28, 250, 12, (const mchar*)ipbuf, &opts, false);

        // MAC row
        char macbuf[18]; macToStr(ns.mac, macbuf);
        target->DrawText(10, 42, 36, 12, "MAC:", &opts, false);
        target->DrawText(50, 42, 250, 12, (const mchar*)macbuf, &opts, false);

        // Driver row
        target->DrawText(10, 56, 60, 12, "Driver:", &opts, false);
        target->DrawText(76, 56, 220, 12,
            ns.drv_active ? "Active" : "Inactive", &opts, false);

        // Separator before port table
        target->FillRect(7, 72, 306, 1, dark, false);

        // Ports section header
        target->DrawText(10, 75, 70, 12, "TCP ports:", &opts, false);

        if (ns.n_ports == 0) {
            target->DrawText(90, 75, 210, 12, "(none registered)", &opts, false);
        } else {
            // Render ports as a space-separated run, wrapping every 8 per row
            static const int COLS = 8;
            for (int i = 0; i < ns.n_ports && i < 16; i++) {
                int row = i / COLS, col = i % COLS;
                Coord ry = 75 + row * 13;
                Coord rx = (col == 0) ? 90 : 90 + col * 36;
                if (col == 0 && row > 0) {
                    // new row label blank
                    rx = 10;
                    // shift the column positions on rows > 0
                    rx = 10 + (i % COLS) * 36;
                }
                char pbuf[6]; u16ToStr(ns.ports[i], pbuf);
                target->DrawText(rx, ry, 35, 12, (const mchar*)pbuf, &opts, false);
            }
        }

        // Back button
        target->FillRect(7, 158, 306, 1, dark, false);
        target->FillRect(120, 161, 80, 13, dark, false);
        target->FillRect(121, 162, 78, 11, light, false);
        opts.foreground      = dark;
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign   = PlatformAlign::Middle;
        target->DrawText(120, 161, 80, 13, "Back", &opts, false);
    }
};

// 
// Window — File Browser  (ScListMounts 0x2C + ScListDirPath 0x2D)
// Top level shows mount points; Enter drills into one; Backspace/[..] returns.
// 

class MountWindow {
public:
    MountWindow() { currentPath[0] = 0; mountRoot[0] = 0; viewFilePath[0] = 0; }

    bool         wantsViewFile = false;
    char         viewFilePath[128];
    unsigned int viewFileSize  = 0;

    static void onEvent(void* instance, struct PlatformWindowInterfaceInputEvent* data) {
        reinterpret_cast<MountWindow*>(instance)->onEvent_(data);
    }
    void SetWindow(PlatformWindow* w) { wnd = w; }

private:
    PlatformWindow* wnd        = nullptr;
    PlatformColor*  dark       = nullptr;
    PlatformColor*  light      = nullptr;
    PlatformFont*   font       = nullptr;
    int             sel        = 0;
    int             scrollTop  = 0;
    bool            atMounts   = true;   // true = mount list, false = dir listing
    char            currentPath[128];
    char            mountRoot[33];       // path of the mount we entered

    MountInfo_T   mounts[8];
    int           nMounts  = 0;
    VfsDirEntry_T entries[64];
    int           nEntries = 0;

    static const int VIS = 10;

    static bool streq(const char* a, const char* b) {
        while (*a && *b && *a == *b) { a++; b++; }
        return *a == 0 && *b == 0;
    }
    int plen() { int i = 0; while (currentPath[i]) i++; return i; }

    static const char* fsType(unsigned char t) {
        if (t == 1) return "rootfs";
        if (t == 2) return "fat12";
        if (t == 3) return "iso9660";
        return "none";
    }

    // Enter a mount — copy its null-terminated path into currentPath & mountRoot
    void enterMount(int mi) {
        if (mi < 0 || mi >= nMounts) return;
        int nl = mounts[mi].path_len < 32 ? mounts[mi].path_len : 32;
        for (int i = 0; i < nl; i++) currentPath[i] = mountRoot[i] = (char)mounts[mi].path[i];
        currentPath[nl] = mountRoot[nl] = 0;
        if (nl == 0) { currentPath[0] = mountRoot[0] = '/'; currentPath[1] = mountRoot[1] = 0; }
        atMounts = false; sel = 0; scrollTop = 0;
    }

    // Go up: return to mount list if at mount root, else strip last path component
    void goUp() {
        if (streq(currentPath, mountRoot)) {
            atMounts = true; sel = 0; scrollTop = 0; return;
        }
        int len = plen(), i = len - 1;
        while (i > 0 && currentPath[i] != '/') i--;
        if (i == 0) currentPath[1] = 0; else currentPath[i] = 0;
        sel = 0; scrollTop = 0;
    }

    // Navigate into a subdirectory entry
    void goInto(int ei) {
        if (ei < 0 || ei >= nEntries || !entries[ei].is_dir) return;
        int cl = plen();
        int nl = entries[ei].name_len < 32 ? entries[ei].name_len : 32;
        if (cl + 1 + nl >= 127) return;
        if (cl == 1) {
            for (int i = 0; i < nl; i++) currentPath[1+i] = (char)entries[ei].name[i];
            currentPath[1+nl] = 0;
        } else {
            currentPath[cl] = '/';
            for (int i = 0; i < nl; i++) currentPath[cl+1+i] = (char)entries[ei].name[i];
            currentPath[cl+1+nl] = 0;
        }
        sel = 0; scrollTop = 0;
    }

    static void u32str(unsigned int n, char* out) {
        if (!n) { out[0] = '0'; out[1] = 0; return; }
        char t[10]; int i = 0;
        while (n) { t[i++] = '0' + n % 10; n /= 10; }
        for (int j = 0; j < i; j++) out[j] = t[i-1-j];
        out[i] = 0;
    }

    void onEvent_(struct PlatformWindowInterfaceInputEvent* data) {
        if (data->type == PlatformWindowInputEventType::OnPaint) {
            OnPaint(data->Data.OnPaint.ctx, data->Data.OnPaint.target);
            return;
        }
        if (data->type != PlatformWindowInputEventType::OnKeyEvent) return;
        auto* key = data->Data.OnKeyEvent.key;
        if (!key->isKeyDown) return;
        if (key->isEscape) { wnd->Close(); return; }

        if (atMounts) {
            // listItems = nMounts; Back at sel==nMounts
            if (key->isArrowUp) {
                if (sel > 0) { sel--; if (sel < scrollTop) scrollTop = sel; }
                wnd->Repaint(); return;
            }
            if (key->isArrowDown) {
                if (sel < nMounts) { sel++; } // nMounts = Back index
                wnd->Repaint(); return;
            }
            if (key->isEnter) {
                if (sel == nMounts) { wnd->Close(); return; }
                enterMount(sel);
                wnd->Repaint();
            }
        } else {
            // listItems = 1 + nEntries ([..] + entries); Back at sel==1+nEntries
            int listItems = 1 + nEntries;
            if (key->isBackspace) { goUp(); wnd->Repaint(); return; }
            if (key->isArrowUp) {
                if (sel > 0) { sel--; if (sel < scrollTop) scrollTop = sel; }
                wnd->Repaint(); return;
            }
            if (key->isArrowDown) {
                if (sel < listItems) {
                    sel++;
                    if (sel < listItems && sel >= scrollTop + VIS) scrollTop = sel - VIS + 1;
                }
                wnd->Repaint(); return;
            }
            if (key->isEnter) {
                if (sel == listItems) { wnd->Close(); return; }
                if (sel == 0) { goUp(); wnd->Repaint(); return; }
                int ei = sel - 1;
                if (entries[ei].is_dir) {
                    goInto(ei); wnd->Repaint();
                } else {
                    // Build full path for read_file: currentPath + "/" + name
                    int cl = plen();
                    int nl = entries[ei].name_len < 32 ? entries[ei].name_len : 32;
                    int p  = 0;
                    for (int i = 0; i < cl; i++) viewFilePath[p++] = currentPath[i];
                    if (cl > 1) viewFilePath[p++] = '/';  // avoid "//" at root
                    for (int i = 0; i < nl; i++) viewFilePath[p++] = (char)entries[ei].name[i];
                    viewFilePath[p] = 0;
                    viewFileSize   = entries[ei].size;
                    wantsViewFile  = true;
                    wnd->Close();
                }
            }
        }
    }

    void drawChrome(PlatformDrawingContext* dc, PlatformBitmap* target,
                    const mchar* title, const mchar* pathLine,
                    PlatformDrawTextOptions& opts, Coord W, Coord H) {
        target->FillRect(0,    0,   W,   H,  dark,  false);
        drawWallpaper(dc, target);
        target->FillRect(0,  H-14,  W,   1,  dark,  false);
        target->FillRect(0,  H-13,  W,  13,  light, false);
        target->FillRect(5,   8,  310, 175,  dark,  false);
        target->FillRect(7,  10,  306, 171,  light, false);
        target->FillRect(7,  24,  306,   1,  dark,  false);
        target->FillRect(7,  37,  306,   1,  dark,  false);
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.foreground      = dark;
        target->DrawText(7,    10, 280, 14, title,         &opts, false);
        target->DrawText(0,  H-13,   W, 13, "Files  -  r2", &opts, false);
        opts.horizontalAlign = PlatformAlign::Begin;
        target->DrawText(10,  25, 290, 12, pathLine, &opts, false);
    }

    void drawBack(PlatformBitmap* target, bool focused, PlatformDrawTextOptions& opts) {
        target->FillRect(7, 158, 306, 1, dark, false);
        if (focused) {
            target->FillRect(120, 161, 80, 13, dark, false);
            opts.foreground = light;
        } else {
            target->FillRect(120, 161, 80, 13, dark, false);
            target->FillRect(121, 162, 78, 11, light, false);
            opts.foreground = dark;
        }
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign   = PlatformAlign::Middle;
        target->DrawText(120, 161, 80, 13, "Back", &opts, false);
    }

    void OnPaint(PlatformDrawingContext* dc, PlatformBitmap* target) {
        if (!target) return;
        if (!dark)  dark  = dc->CreateColor(0xFF0A0A20, nullptr, nullptr);
        if (!light) light = dc->CreateColor(0xFFE0E0FF, nullptr, nullptr);
        if (!font)  font  = dc->CreateFont(12, nullptr, false, false, false, nullptr, nullptr);
        if (!dark || !light || !font) return;

        Coord W = target->GetWidth(), H = target->GetHeight();
        PlatformDrawTextOptions opts{};
        opts.font          = font;
        opts.verticalAlign = PlatformAlign::Middle;

        if (atMounts) {
            // --- Mount list view ---
            int raw = (int)list_mounts(mounts);
            nMounts = raw > 0 ? raw : 0;
            if (sel > nMounts) sel = nMounts;

            drawChrome(dc, target, "Files", "Mount Points", opts, W, H);

            if (nMounts == 0) {
                opts.horizontalAlign = PlatformAlign::Middle;
                opts.foreground      = dark;
                target->DrawText(8, 38, 304, 11, "(no mounts)", &opts, false);
            } else {
                for (int row = 0; row < VIS && row < nMounts; row++) {
                    Coord ry = 38 + row * 12;
                    bool isSel = (sel == row);
                    if (isSel) { target->FillRect(8, ry, 304, 11, dark, false); opts.foreground = light; }
                    else        { opts.foreground = dark; }
                    // Mount path (null-terminate)
                    char pb[33]; int pl = mounts[row].path_len < 32 ? mounts[row].path_len : 32;
                    for (int j = 0; j < pl; j++) pb[j] = (char)mounts[row].path[j]; pb[pl] = 0;
                    if (pl == 0) { pb[0] = '/'; pb[1] = 0; }
                    opts.horizontalAlign = PlatformAlign::Begin;
                    target->DrawText(9,   ry, 180, 11, (const mchar*)pb,              &opts, false);
                    target->DrawText(210, ry,  90, 11, (const mchar*)fsType(mounts[row].fs_type), &opts, false);
                }
            }
            drawBack(target, sel == nMounts, opts);

        } else {
            // --- Directory listing view ---
            int raw = (int)list_dir_path((const unsigned char*)currentPath, entries);
            if (raw < 0) raw = 0;
            nEntries = 0;
            for (int i = 0; i < raw; i++) {
                unsigned char nl = entries[i].name_len;
                if (nl == 1 && entries[i].name[0] == '.') continue;
                if (nl == 2 && entries[i].name[0] == '.' && entries[i].name[1] == '.') continue;
                if (nEntries != i) entries[nEntries] = entries[i];
                nEntries++;
            }
            int listItems = 1 + nEntries;  // [..] + entries; Back at sel==listItems
            if (sel > listItems) sel = listItems;

            drawChrome(dc, target, "Files", (const mchar*)currentPath, opts, W, H);

            for (int row = 0; row < VIS; row++) {
                int vi = scrollTop + row;
                if (vi >= listItems) break;
                Coord ry = 38 + row * 12;
                bool isSel = (sel == vi);
                if (isSel) { target->FillRect(8, ry, 304, 11, dark, false); opts.foreground = light; }
                else        { opts.foreground = dark; }
                opts.horizontalAlign = PlatformAlign::Begin;
                if (vi == 0) {
                    target->DrawText(9, ry, 290, 11, "[..]", &opts, false);
                } else {
                    int ei = vi - 1;
                    char nb[33]; int nl = entries[ei].name_len < 32 ? entries[ei].name_len : 32;
                    for (int j = 0; j < nl; j++) nb[j] = (char)entries[ei].name[j]; nb[nl] = 0;
                    if (entries[ei].is_dir) {
                        target->DrawText(9,  ry,   8, 11, "/",              &opts, false);
                        target->DrawText(17, ry, 190, 11, (const mchar*)nb, &opts, false);
                    } else {
                        target->DrawText(17, ry, 175, 11, (const mchar*)nb, &opts, false);
                        char sb[12]; u32str(entries[ei].size, sb);
                        target->DrawText(222, ry, 78,  11, (const mchar*)sb, &opts, false);
                    }
                }
            }
            drawBack(target, sel == listItems, opts);
        }
    }
};

//
// Window — File Viewer  (ScReadFile 0x20 + ScChdir 0x2E)
// Displays the text content of a file selected in MountWindow.
// read_file only searches one directory level, so we chdir to the parent
// dir first, then call read_file with just the filename component.
//

class FileViewerWindow {
public:
    // 32 KB of visible content + 512-byte pad for last FAT sector overflow
    static const int MAX_FILE  = 32768;
    static const int BUF_SIZE  = MAX_FILE + 512;
    static const int MAX_LINES = 256;
    static const int VIS       = 10;

    FileViewerWindow(const char* path, unsigned int size) {
        int pi = 0;
        while (path[pi] && pi < 127) { filePath[pi] = path[pi]; pi++; }
        filePath[pi] = 0;

        nLines = 0; scrollTop = 0; fileBytes = 0;

        if (size == 0) {
            setMsg("(empty file)");
        } else if (size > (unsigned int)MAX_FILE) {
            setMsg("(file too large to display)");
        } else {
#ifdef MEMENTO_BACKEND_R2
            long ret = 0;

            // ISO9660: kernel read_file accepts the full absolute path directly
            // (try_iso9660_absolute strips the mount prefix and walks the directory).
            // FAT12: fat83() is a single-component converter, so we must chdir to
            // the parent directory and pass only the bare filename to read_file.
            {
                const char* iso_pfx = "/mnt/iso";
                int j = 0;
                while (iso_pfx[j] && path[j] == iso_pfx[j]) j++;
                bool is_iso = (!iso_pfx[j] && (path[j] == '/' || path[j] == 0));

                if (is_iso) {
                    ret = read_file((const unsigned char*)path, (unsigned char*)content);
                } else {
                    int last = 0;
                    for (int i = 0; path[i]; i++) if (path[i] == '/') last = i;
                    char parentBuf[128];
                    int  pl = (last == 0) ? 1 : last;
                    for (int i = 0; i < pl; i++) parentBuf[i] = path[i];
                    parentBuf[pl] = 0;
                    chdir((const unsigned char*)parentBuf);
                    ret = read_file((const unsigned char*)(path + last + 1), (unsigned char*)content);
                }
            }

            // libcr2 read_file: returns 1 on success, 0 on failure
            if (ret != 0) {
                fileBytes = size;
            } else {
                setMsg("(read error)");
            }
#else
            setMsg("(read_file not available on this platform)");
#endif
        }
        content[fileBytes] = 0;
        buildLines();
    }

    static void onEvent(void* instance, struct PlatformWindowInterfaceInputEvent* data) {
        reinterpret_cast<FileViewerWindow*>(instance)->onEvent_(data);
    }
    void SetWindow(PlatformWindow* w) { wnd = w; }

private:
    PlatformWindow* wnd   = nullptr;
    PlatformColor*  dark  = nullptr;
    PlatformColor*  light = nullptr;
    PlatformFont*   font  = nullptr;

    char         filePath[128];
    unsigned int fileBytes  = 0;
    int          scrollTop  = 0;
    int          nLines     = 0;

    // Static: only one FileViewerWindow open at a time; keeps heap object small.
    static char content[BUF_SIZE + 1];
    static int  lineStart[MAX_LINES];
    static int  lineLen[MAX_LINES];

    void setMsg(const char* msg) {
        int i = 0;
        while (msg[i] && i < MAX_FILE) { content[i] = msg[i]; i++; }
        fileBytes = i;
    }

    void buildLines() {
        nLines = 0;
        int ls = 0, i = 0;
        while (i <= (int)fileBytes && nLines < MAX_LINES) {
            if (i == (int)fileBytes || content[i] == '\n') {
                int len = i - ls;
                while (len > 0 && content[ls + len - 1] == '\r') len--;
                lineStart[nLines] = ls;
                lineLen[nLines]   = len;
                nLines++;
                ls = i + 1;
            }
            i++;
        }
        if (nLines == 0) { lineStart[0] = 0; lineLen[0] = 0; nLines = 1; }
    }

    void clampScroll() {
        int maxTop = nLines - VIS;
        if (maxTop < 0) maxTop = 0;
        if (scrollTop > maxTop) scrollTop = maxTop;
        if (scrollTop < 0)      scrollTop = 0;
    }

    void onEvent_(struct PlatformWindowInterfaceInputEvent* data) {
        if (data->type == PlatformWindowInputEventType::OnPaint) {
            OnPaint(data->Data.OnPaint.ctx, data->Data.OnPaint.target); return;
        }
        if (data->type != PlatformWindowInputEventType::OnKeyEvent) return;
        auto* key = data->Data.OnKeyEvent.key;
        if (!key->isKeyDown) return;
        if (key->isEscape || key->isEnter) { wnd->Close(); return; }
        if (key->isArrowUp)   { scrollTop--;        clampScroll(); wnd->Repaint(); return; }
        if (key->isArrowDown) { scrollTop++;        clampScroll(); wnd->Repaint(); return; }
        if (key->isPageUp)    { scrollTop -= VIS;   clampScroll(); wnd->Repaint(); return; }
        if (key->isPageDown)  { scrollTop += VIS;   clampScroll(); wnd->Repaint(); return; }
    }

    void OnPaint(PlatformDrawingContext* dc, PlatformBitmap* target) {
        if (!target) return;
        if (!dark)  dark  = dc->CreateColor(0xFF0A0A20, nullptr, nullptr);
        if (!light) light = dc->CreateColor(0xFFE0E0FF, nullptr, nullptr);
        if (!font)  font  = dc->CreateFont(12, nullptr, false, false, false, nullptr, nullptr);
        if (!dark || !light || !font) return;

        Coord W = target->GetWidth(), H = target->GetHeight();

        target->FillRect(0,    0,   W,   H,  dark,  false);
        drawWallpaper(dc, target);
        target->FillRect(0,  H-14,  W,   1,  dark,  false);
        target->FillRect(0,  H-13,  W,  13,  light, false);
        target->FillRect(5,   8,  310, 175,  dark,  false);
        target->FillRect(7,  10,  306, 171,  light, false);
        target->FillRect(7,  24,  306,   1,  dark,  false);
        target->FillRect(7,  37,  306,   1,  dark,  false);

        PlatformDrawTextOptions opts{};
        opts.font            = font;
        opts.foreground      = dark;
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign   = PlatformAlign::Middle;
        target->DrawText(7,    10, 280, 14, "File Viewer",  &opts, false);
        target->DrawText(0,  H-13,   W, 13, "File  -  r2", &opts, false);

        // Path on left, line/total on right of the subheader row
        opts.horizontalAlign = PlatformAlign::Begin;
        target->DrawText(10, 25, 220, 12, (const mchar*)filePath, &opts, false);

        if (nLines > VIS) {
            // "line / total" indicator, e.g. "12/47"
            char sbuf[16];
            int sn = 0;
            auto writeInt = [&](int v) {
                if (v == 0) { sbuf[sn++] = '0'; return; }
                char tmp[6]; int ti = 0;
                while (v > 0) { tmp[ti++] = '0' + v % 10; v /= 10; }
                for (int j = ti - 1; j >= 0; j--) sbuf[sn++] = tmp[j];
            };
            writeInt(scrollTop + 1);
            sbuf[sn++] = '/';
            writeInt(nLines);
            sbuf[sn] = 0;
            opts.horizontalAlign = PlatformAlign::End;
            target->DrawText(10, 25, 300, 12, (const mchar*)sbuf, &opts, false);
        }

        opts.horizontalAlign = PlatformAlign::Begin;
        for (int row = 0; row < VIS; row++) {
            int li = scrollTop + row;
            if (li >= nLines) break;
            Coord ry = 38 + row * 12;
            char  lineBuf[128];
            int   ll = lineLen[li] < 127 ? lineLen[li] : 127;
            for (int j = 0; j < ll; j++) {
                char c = content[lineStart[li] + j];
                lineBuf[j] = (c >= 0x20 && c < 0x7F) ? c : '.';
            }
            lineBuf[ll] = 0;
            target->DrawText(9, ry, 302, 11, (const mchar*)lineBuf, &opts, false);
        }

        // Back button
        target->FillRect(7,   158, 306,  1, dark,  false);
        target->FillRect(120, 161,  80, 13, dark,  false);
        target->FillRect(121, 162,  78, 11, light, false);
        opts.foreground      = dark;
        opts.horizontalAlign = PlatformAlign::Middle;
        target->DrawText(120, 161, 80, 13, "Back", &opts, false);
    }
};

char FileViewerWindow::content[FileViewerWindow::BUF_SIZE + 1];
int  FileViewerWindow::lineStart[FileViewerWindow::MAX_LINES];
int  FileViewerWindow::lineLen[FileViewerWindow::MAX_LINES];

//
// Window 5 — Desktop launcher
//

class DesktopWindow {
public:
    static void onEvent(void* instance, struct PlatformWindowInterfaceInputEvent* data) {
        reinterpret_cast<DesktopWindow*>(instance)->onEvent_(data);
    }

    void SetWindow(PlatformWindow* w) { wnd = w; }
    bool wantsTasks = false;
    bool wantsMount = false;
    bool wantsNet   = false;
    bool wantsShell = false;

private:
    PlatformWindow* wnd   = nullptr;
    PlatformColor*  dark  = nullptr;
    PlatformColor*  light = nullptr;
    PlatformFont*   font  = nullptr;
    int             sel   = 0;  // 0=Clock, 1=Shell, 2=Net, 3=Mount, 4=Tasks

    // Icon bitmaps are created at Coord(32)×Coord(32); at DPI=86 that is
    // ceil(32·86/96)=29 raw pixels.  All screen positions use Dim (raw pixels)
    // sized for the DPI=86 inner dialog area (≈265×104 px at x=11,y=22).
    static const int BSIZ = 29;   // icon pixel size at DPI=86
    static const int IX0  = 31, IX1 = 80, IX2 = 129, IX3 = 178, IX4 = 227;
    static const int IY   = 41;   // icon top y (px below title separator)
    static const int LY   = 73;   // label top y  (IY + BSIZ + 3)
    static const int LW   = 48;   // label box width
    static const int LH   = 12;   // label box height

    PlatformBitmap* bmpClock  = nullptr;
    PlatformBitmap* bmpShell  = nullptr;
    PlatformBitmap* bmpNet    = nullptr;
    PlatformBitmap* bmpMount  = nullptr;
    PlatformBitmap* bmpTasks  = nullptr;

    void MakeBitmaps(PlatformDrawingContext* dc) {
        // Clock: circular face, corner roundoff, tick marks, hands
        if (!bmpClock) {
            bmpClock = dc->CreateBitmap(Coord(32), Coord(32), nullptr, nullptr);
            if (bmpClock) {
                bmpClock->FillRectD(Dim(0),  Dim(0),  Dim(29), Dim(29), dark);   // bg
                bmpClock->FillRectD(Dim(3),  Dim(3),  Dim(23), Dim(23), light);  // face
                bmpClock->FillRectD(Dim(3),  Dim(3),  Dim(3),  Dim(3),  dark);   // corner TL
                bmpClock->FillRectD(Dim(23), Dim(3),  Dim(3),  Dim(3),  dark);   // corner TR
                bmpClock->FillRectD(Dim(3),  Dim(23), Dim(3),  Dim(3),  dark);   // corner BL
                bmpClock->FillRectD(Dim(23), Dim(23), Dim(3),  Dim(3),  dark);   // corner BR
                bmpClock->FillRectD(Dim(12), Dim(4),  Dim(5),  Dim(2),  dark);   // 12 tick
                bmpClock->FillRectD(Dim(23), Dim(12), Dim(2),  Dim(5),  dark);   // 3  tick
                bmpClock->FillRectD(Dim(12), Dim(23), Dim(5),  Dim(2),  dark);   // 6  tick
                bmpClock->FillRectD(Dim(4),  Dim(12), Dim(2),  Dim(5),  dark);   // 9  tick
                bmpClock->FillRectD(Dim(13), Dim(7),  Dim(2),  Dim(7),  dark);   // hour hand
                bmpClock->FillRectD(Dim(14), Dim(13), Dim(7),  Dim(2),  dark);   // min  hand
                bmpClock->FillRectD(Dim(13), Dim(13), Dim(2),  Dim(2),  dark);   // pivot
            }
        }
        // Shell: terminal window with title bar dots and ">_" prompt
        if (!bmpShell) {
            bmpShell = dc->CreateBitmap(Coord(32), Coord(32), nullptr, nullptr);
            if (bmpShell) {
                PlatformDrawTextOptions to{};
                to.font = font; to.foreground = light;
                to.horizontalAlign = PlatformAlign::Begin;
                to.verticalAlign   = PlatformAlign::Begin;
                bmpShell->FillRectD(Dim(0),  Dim(0),  Dim(29), Dim(29), dark);   // bg
                bmpShell->FillRectD(Dim(2),  Dim(2),  Dim(25), Dim(5),  light);  // title bar
                bmpShell->FillRectD(Dim(4),  Dim(3),  Dim(3),  Dim(3),  dark);   // dot 1
                bmpShell->FillRectD(Dim(9),  Dim(3),  Dim(3),  Dim(3),  dark);   // dot 2
                bmpShell->FillRectD(Dim(14), Dim(3),  Dim(3),  Dim(3),  dark);   // dot 3
                bmpShell->DrawTextD(Dim(3),  Dim(9),  Dim(24), Dim(16), ">_", &to);
            }
        }
        // Net: parabolic dish (opens right) + signal glyphs << / >>
        if (!bmpNet) {
            bmpNet = dc->CreateBitmap(Coord(32), Coord(32), nullptr, nullptr);
            if (bmpNet) {
                PlatformDrawTextOptions to{};
                to.font = font; to.foreground = light;
                to.horizontalAlign = PlatformAlign::Begin;
                to.verticalAlign   = PlatformAlign::Begin;
                bmpNet->FillRectD(Dim(0),  Dim(0),  Dim(29), Dim(29), dark);
                bmpNet->FillRectD(Dim(10), Dim(3),  Dim(4),  Dim(2),  light);  // top arm
                bmpNet->FillRectD(Dim(7),  Dim(5),  Dim(4),  Dim(2),  light);  // curve
                bmpNet->FillRectD(Dim(5),  Dim(7),  Dim(3),  Dim(2),  light);  // curve
                bmpNet->FillRectD(Dim(3),  Dim(9),  Dim(3),  Dim(2),  light);  // curve
                bmpNet->FillRectD(Dim(2),  Dim(11), Dim(3),  Dim(4),  light);  // apex
                bmpNet->FillRectD(Dim(3),  Dim(15), Dim(3),  Dim(2),  light);  // curve
                bmpNet->FillRectD(Dim(5),  Dim(17), Dim(3),  Dim(2),  light);  // curve
                bmpNet->FillRectD(Dim(7),  Dim(19), Dim(4),  Dim(2),  light);  // curve
                bmpNet->FillRectD(Dim(10), Dim(21), Dim(4),  Dim(2),  light);  // bottom arm
                bmpNet->FillRectD(Dim(9),  Dim(23), Dim(5),  Dim(2),  light);  // base top
                bmpNet->FillRectD(Dim(7),  Dim(25), Dim(7),  Dim(2),  light);  // base mid
                bmpNet->FillRectD(Dim(5),  Dim(27), Dim(10), Dim(2),  light);  // base foot
                bmpNet->DrawTextD(Dim(15), Dim(3),  Dim(12), Dim(10), "<<",    &to);
                bmpNet->DrawTextD(Dim(15), Dim(13), Dim(12), Dim(10), ">>",    &to);
            }
        }
        // Mount: three stacked bars with left-side label dot
        if (!bmpMount) {
            bmpMount = dc->CreateBitmap(Coord(32), Coord(32), nullptr, nullptr);
            if (bmpMount) {
                bmpMount->FillRectD(Dim(0),  Dim(0),  Dim(29), Dim(29), dark);
                bmpMount->FillRectD(Dim(3),  Dim(4),  Dim(23), Dim(5),  light);  // bar 1
                bmpMount->FillRectD(Dim(3),  Dim(12), Dim(23), Dim(5),  light);  // bar 2
                bmpMount->FillRectD(Dim(3),  Dim(20), Dim(23), Dim(5),  light);  // bar 3
                bmpMount->FillRectD(Dim(5),  Dim(6),  Dim(4),  Dim(2),  dark);   // dot 1
                bmpMount->FillRectD(Dim(5),  Dim(14), Dim(4),  Dim(2),  dark);   // dot 2
                bmpMount->FillRectD(Dim(5),  Dim(22), Dim(4),  Dim(2),  dark);   // dot 3
            }
        }
        // Tasks: bar-graph with 4 bars of varying height over a baseline
        if (!bmpTasks) {
            bmpTasks = dc->CreateBitmap(Coord(32), Coord(32), nullptr, nullptr);
            if (bmpTasks) {
                bmpTasks->FillRectD(Dim(0),  Dim(0),  Dim(29), Dim(29), dark);
                bmpTasks->FillRectD(Dim(3),  Dim(18), Dim(4),  Dim(7),  light);  // bar 1
                bmpTasks->FillRectD(Dim(9),  Dim(11), Dim(4),  Dim(14), light);  // bar 2
                bmpTasks->FillRectD(Dim(15), Dim(14), Dim(4),  Dim(11), light);  // bar 3
                bmpTasks->FillRectD(Dim(22), Dim(7),  Dim(4),  Dim(18), light);  // bar 4
                bmpTasks->FillRectD(Dim(3),  Dim(25), Dim(23), Dim(2),  light);  // baseline
            }
        }
    }

    void BlitIcon(PlatformBitmap* t, PlatformBitmap* bm, int ix, int iy, bool s) {
        if (s) t->FillRectD(Dim(ix - 2), Dim(iy - 2), Dim(BSIZ + 4), Dim(BSIZ + 4), dark);
        if (bm)
            t->CopyBitmapD(Dim(ix), Dim(iy), Dim(BSIZ), Dim(BSIZ),
                           bm, Dim(0), Dim(0), Dim(BSIZ), Dim(BSIZ), false, 255);
    }

    void onEvent_(struct PlatformWindowInterfaceInputEvent* data) {
        if (data->type == PlatformWindowInputEventType::OnPaint) {
            OnPaint(data->Data.OnPaint.ctx, data->Data.OnPaint.target);
            return;
        }
        if (data->type != PlatformWindowInputEventType::OnKeyEvent) return;
        auto* key = data->Data.OnKeyEvent.key;
        if (!key->isKeyDown) return;
        if (key->isEscape) { wnd->Close(); return; }
        if (key->isArrowLeft  || key->isArrowUp)   { sel = (sel + 4) % 5; wnd->Repaint(); return; }
        if (key->isArrowRight || key->isArrowDown) { sel = (sel + 1) % 5; wnd->Repaint(); return; }
        if (key->isEnter) {
            if (sel == 1) wantsShell = true;
            if (sel == 2) wantsNet   = true;
            if (sel == 3) wantsMount = true;
            if (sel == 4) wantsTasks = true;
            wnd->Close();
        }
    }

    void OnPaint(PlatformDrawingContext* dc, PlatformBitmap* target) {
        if (!target) return;
        if (!dark)  dark  = dc->CreateColor(0xFF0A0A20, nullptr, nullptr);
        if (!light) light = dc->CreateColor(0xFFE0E0FF, nullptr, nullptr);
        if (!font)  font  = dc->CreateFont(12, nullptr, false, false, false, nullptr, nullptr);
        if (!dark || !light || !font) return;
        MakeBitmaps(dc);

        Coord W = target->GetWidth();
        Coord H = target->GetHeight();

        target->FillRect(0, 0, W, H, dark, false);
        drawWallpaper(dc, target);

        // Taskbar
        target->FillRect(0, H - 14, W,  1, dark,  false);
        target->FillRect(0, H - 13, W, 13, light, false);

        // Dialog frame
        target->FillRect(10, 22, 300, 120, dark,  false);
        target->FillRect(12, 24, 296, 116, light, false);
        target->FillRectD(Dim(11), Dim(35), Dim(265), Dim(1), dark);  // title separator

        PlatformDrawTextOptions opts{};
        opts.font            = font;
        opts.foreground      = dark;
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign   = PlatformAlign::Middle;

        target->DrawText(12,    24, 270,  14, "Desktop",        &opts, false);
        target->DrawText(0, H - 13,   W,  13, "Desktop  -  r2", &opts, false);

        // Icons — blitted from pre-drawn bitmaps at exact Dim pixel positions
        BlitIcon(target, bmpClock, IX0, IY, sel == 0);
        BlitIcon(target, bmpShell, IX1, IY, sel == 1);
        BlitIcon(target, bmpNet,   IX2, IY, sel == 2);
        BlitIcon(target, bmpMount, IX3, IY, sel == 3);
        BlitIcon(target, bmpTasks, IX4, IY, sel == 4);

        // Labels centred on each icon
        PlatformDrawTextOptions lo{};
        lo.font            = font;
        lo.foreground      = dark;
        lo.horizontalAlign = PlatformAlign::Middle;
        lo.verticalAlign   = PlatformAlign::Middle;
        const int loff = (LW - BSIZ) / 2;   // 9 px: centres LW box on BSIZ icon
        target->DrawTextD(Dim(IX0 - loff), Dim(LY), Dim(LW), Dim(LH), "Clock", &lo);
        target->DrawTextD(Dim(IX1 - loff), Dim(LY), Dim(LW), Dim(LH), "Shell", &lo);
        target->DrawTextD(Dim(IX2 - loff), Dim(LY), Dim(LW), Dim(LH), "Net",   &lo);
        target->DrawTextD(Dim(IX3 - loff), Dim(LY), Dim(LW), Dim(LH), "Mount", &lo);
        target->DrawTextD(Dim(IX4 - loff), Dim(LY), Dim(LW), Dim(LH), "Tasks", &lo);
    }
};

// 
// Entry point — window objects are heap-allocated to keep the stack lean
// 

extern "C" int main() {
    UIRootImpl* root = new UIRootImpl();
    if (!root || root->HasError()) return 1;

    UIRootImpl::PlatformWindowOptions opts{};
    opts.initialVisible = true;
    opts.useCustomDPI   = true;
    opts.customDPI      = 86; // 96, 86, 64, 125 (120% zoom-in)

    // --- Window 1 ---
    HelloWindow* hw = new HelloWindow();
    PlatformWindow* wnd = root->CreateWindow(
        "Hello r2", 0, 0,
        HelloWindow::onEvent, hw,
        &opts, nullptr, nullptr);
    if (!wnd) return 1;
    hw->SetWindow(wnd);
    wnd->SetVisible(true);
    root->EnterMainLoop();

    // --- Window 2: Login ---
    if (hw->wantsNext) {
        LoginWindow* lw = new LoginWindow();
        PlatformWindow* wnd2 = root->CreateWindow(
            "Login", 0, 0,
            LoginWindow::onEvent, lw,
            &opts, nullptr, nullptr);
        if (wnd2) {
            lw->SetWindow(wnd2);
            wnd2->SetVisible(true);
            root->EnterMainLoop();
        }

        // --- Windows 3+: Desktop ↔ Tasks loop ---
        bool showDesktop = lw->wantsDesktop;
#ifdef MEMENTO_BACKEND_R2
        unsigned long heapMark = r2_heap_checkpoint();
#endif
        while (showDesktop) {
            showDesktop = false;
#ifdef MEMENTO_BACKEND_R2
            r2_heap_restore(heapMark);  // reclaim previous iteration's windows
#endif
            DesktopWindow* desk = new DesktopWindow();
            PlatformWindow* wnd3 = root->CreateWindow(
                "Desktop", 0, 0,
                DesktopWindow::onEvent, desk,
                &opts, nullptr, nullptr);
            if (!wnd3) break;
            desk->SetWindow(wnd3);
            wnd3->SetVisible(true);
            root->EnterMainLoop();

            if (desk->wantsShell) {
#ifdef MEMENTO_BACKEND_R2
                set_video_mode(0x03);
                unsigned char sh_pid = 0;
                if (run_elf((const unsigned char*)"sh.elf",
                            (const unsigned char*)"sh.elf", &sh_pid)) {
                    int misses = 0;
                    while (true) {
                        sleep_ms(500);
                        TaskInfo_T tasks[16];
                        long n = list_tasks(tasks, 16);
                        if (n <= 0) continue;  // try_lock failed, keep waiting
                        bool alive = false;
                        for (long i = 0; i < n; i++) {
                            // Match by name ("SH.ELF" → name[0]='S', name[1]='H')
                            // and status < 4 (Ready/Running/Idle/Blocked)
                            if (tasks[i].name[0] == 'S' && tasks[i].name[1] == 'H'
                                    && tasks[i].status < 4) {
                                alive = true;
                                break;
                            }
                        }
                        if (alive) { misses = 0; continue; }
                        if (++misses >= 3) break;  // 3 consecutive misses = shell gone
                    }
                }
                set_video_mode(0x13);
#endif
                showDesktop = true;
            } else if (desk->wantsNet) {
                NetWindow* nw = new NetWindow();
                PlatformWindow* wndN = root->CreateWindow(
                    "Network", 0, 0,
                    NetWindow::onEvent, nw,
                    &opts, nullptr, nullptr);
                if (wndN) {
                    nw->SetWindow(wndN);
                    wndN->SetVisible(true);
                    root->EnterMainLoop();
                }
                showDesktop = true;
            } else if (desk->wantsTasks) {
                TasksWindow* tw = new TasksWindow();
                PlatformWindow* wnd4 = root->CreateWindow(
                    "Tasks", 0, 0,
                    TasksWindow::onEvent, tw,
                    &opts, nullptr, nullptr);
                if (wnd4) {
                    tw->SetWindow(wnd4);
                    wnd4->SetVisible(true);
                    root->EnterMainLoop();
                }
                showDesktop = true;
            } else if (desk->wantsMount) {
#ifdef MEMENTO_BACKEND_R2
                unsigned long mountMark = r2_heap_checkpoint();
#endif
                bool showMount = true;
                while (showMount) {
                    showMount = false;
#ifdef MEMENTO_BACKEND_R2
                    r2_heap_restore(mountMark);
#endif
                    MountWindow* mw = new MountWindow();
                    PlatformWindow* wnd5 = root->CreateWindow(
                        "Files", 0, 0, MountWindow::onEvent, mw, &opts, nullptr, nullptr);
                    if (wnd5) {
                        mw->SetWindow(wnd5);
                        wnd5->SetVisible(true);
                        root->EnterMainLoop();
                    }
                    if (mw->wantsViewFile) {
                        // Copy path to stack before heap restore frees mw
                        char vpath[128] = {};
                        unsigned int vsz = mw->viewFileSize;
                        for (int i = 0; mw->viewFilePath[i] && i < 127; i++)
                            vpath[i] = mw->viewFilePath[i];
#ifdef MEMENTO_BACKEND_R2
                        r2_heap_restore(mountMark);
#endif
                        FileViewerWindow* fvw = new FileViewerWindow(vpath, vsz);
                        PlatformWindow* wndF = root->CreateWindow(
                            "File", 0, 0, FileViewerWindow::onEvent, fvw, &opts, nullptr, nullptr);
                        if (wndF) {
                            fvw->SetWindow(wndF);
                            wndF->SetVisible(true);
                            root->EnterMainLoop();
                        }
                        showMount = true;  // return to file browser after viewing
                    }
                }
                showDesktop = true;
            }
            // ESC on desktop → showDesktop stays false → exit loop
        }
    }

#ifdef MEMENTO_BACKEND_R2
    set_video_mode(0x03);
#endif
    return 0;
}
