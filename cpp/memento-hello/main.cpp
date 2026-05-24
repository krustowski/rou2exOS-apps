#include "ui/platform/impl/UIImpl.h"
#include "ui/platform/PlatformWindow.h"
#include "ui/platform/PlatformKey.h"
#include "ui/platform/PlatformDrawingContext.h"
#include "ui/platform/PlatformBitmap.h"
#include "ui/platform/PlatformColor.h"
#include "ui/platform/PlatformFont.h"

using namespace Memento;

#ifdef MEMENTO_BACKEND_R2
extern "C"
{
    long set_video_mode(unsigned char mode);

    // ScListTasks (0x2F) — forward-declare to avoid FBInfo_T conflict with R2_LL.h
    struct TaskInfo_T
    {
        unsigned char id, mode, status, _pad;
        unsigned char name[16];
    } __attribute__((packed));
    long list_tasks(TaskInfo_T *buf, unsigned char max);

    // ScNetStatus (0x38)
    struct NetStatus_T
    {
        unsigned char mac[6];
        unsigned char ip[4];
        unsigned char drv_active;
        unsigned char n_ports;
        unsigned short ports[16];
    } __attribute__((packed));
    long get_net_status(NetStatus_T *ns);

    // ScListMounts (0x2C)
    struct MountInfo_T
    {
        unsigned char path[32];
        unsigned char path_len;
        unsigned char fs_type;
    } __attribute__((packed));
    long list_mounts(MountInfo_T *buf);

    // ScListDirPath (0x2D)
    struct VfsDirEntry_T
    {
        unsigned char name[32];
        unsigned char name_len;
        unsigned char is_dir;
        unsigned int size;
    } __attribute__((packed));
    long list_dir_path(const unsigned char *path, VfsDirEntry_T *buf);

    // ScReadFile (0x20)
    long read_file(const unsigned char *name, unsigned char *buf);

    // ScChdir (0x2E)
    long chdir(const unsigned char *path);

    // Heap checkpoint/restore for the Desktop navigation loop
    unsigned long r2_heap_checkpoint();
    void r2_heap_restore(unsigned long cp);

    long run_elf(const unsigned char *name, const unsigned char *args, unsigned char *pid);
    void sleep_ms(unsigned long long ms);
    unsigned long get_ticks();

    // ScRTC (0x02) — real-time clock
    struct RTC_raw
    {
        unsigned char seconds, minutes, hours, day, month;
        unsigned short year;
    } __attribute__((packed));
    long read_rtc(RTC_raw *rtc);

    // ScSysInfo (0x01) — system config read/write
    struct SysInfo_T
    {
        unsigned char system_name[32];
        unsigned char system_user[32];
        unsigned char system_path[32];
        unsigned char system_version[8];
        unsigned int system_path_cluster;
        unsigned int system_uptime;
        unsigned char ip_addr[4];
    } __attribute__((packed));
    long read_sysinfo(SysInfo_T *sysinfo);

// TCP networking (from libcr2/net.h) — types forward-declared to avoid pulling
// in the full libcr2 headers which redefine uint8_t etc.
#define CHAT_PORT_C 9000
#define MAX_SOCKETS_C 8

    struct TcpSocket_C
    {
        unsigned int id;
        int state; // SocketState enum: 0=CLOSED..3=ESTABLISHED..4=FIN_WAIT
        unsigned short local_port;
        unsigned short remote_port;
        unsigned char local_ip[4];
        unsigned char remote_ip[4];
        unsigned char rx_buffer[1024];
        unsigned char tx_buffer[1024];
        unsigned int rx_len;
        unsigned int tx_len;
        unsigned char used;
        unsigned int seq_num;
        unsigned int ack_num;
    } __attribute__((packed));

    struct Ipv4Header_C
    {
        unsigned char version;
        unsigned char dscp_ecn;
        unsigned short total_length;
        unsigned short identification;
        unsigned short flags_fragment_offset;
        unsigned char ttl;
        unsigned char protocol;
        unsigned short header_checksum;
        unsigned char source_addr[4];
        unsigned char destination_addr[4];
    } __attribute__((packed));

    struct TcpHeader_C
    {
        unsigned short source_port;
        unsigned short dest_port;
        unsigned int seq_num;
        unsigned int ack_num;
        unsigned short data_offset_reserved_flags;
        unsigned short window_size;
        unsigned short checksum;
        unsigned short urgent_pointer;
    } __attribute__((packed));

    struct NetDriver_C
    {
        int (*recv)(unsigned char *buf, unsigned int maxlen);
        void (*send_ip)(const unsigned char *ip_pkt, unsigned int len);
    };
    extern NetDriver_C net_drv;

    int net_driver_select(const unsigned char *name);
    int net_driver_bind_port(const unsigned char *name, unsigned short port);
    void net_get_local_ip(unsigned char ip[4]);
    long send_eth_frame(const unsigned char *frame, unsigned int len);
    TcpSocket_C *tcp_connect(TcpSocket_C sockets[MAX_SOCKETS_C],
                             const unsigned char remote_ip[4], unsigned short remote_port,
                             unsigned short local_port, const unsigned char local_ip[4]);
    void on_tcp_packet(const unsigned char src_ip[4], const unsigned char dst_ip[4],
                       TcpHeader_C *tcp_header, const unsigned char *payload, unsigned int len,
                       TcpSocket_C sockets[MAX_SOCKETS_C]);
    unsigned short parse_ipv4_packet(const unsigned char *packet, Ipv4Header_C *header);
    unsigned short parse_tcp_packet(const unsigned char *packet, TcpHeader_C *header);
    // send_tcp_packet: retransmit SYN after ARP cache is populated
    void send_tcp_packet_c(TcpSocket_C *sock, const unsigned char *data,
                           unsigned int len, unsigned char flags) __asm__("send_tcp_packet");
    // read/write/close aliased to avoid clashing with any POSIX declarations
    unsigned int chat_sock_read(TcpSocket_C *sock, unsigned char *buf, unsigned int maxlen) __asm__("read");
    unsigned int chat_sock_write(TcpSocket_C *sock, const unsigned char *buf, unsigned int len) __asm__("write");
    void chat_sock_close(TcpSocket_C *sock) __asm__("close");
    // net_recv_nb: non-blocking receive — returns 0 immediately if no frame queued
    int net_recv_nb(unsigned char *buf, unsigned int maxlen);
    void net_arp_set(const unsigned char ip[4], const unsigned char mac[6]);
}
#endif

#include "windows/hello_window.cpp"
#include "windows/wallpaper.cpp"
#include "windows/login_window.cpp"
#include "windows/tasks_window.cpp"
#include "windows/net_window.cpp"
#include "windows/mount_window.cpp"
#include "windows/file_viewer_window.cpp"
#include "windows/chat_window.cpp"
#include "windows/calculator_window.cpp"
#include "windows/clock_window.cpp"
#include "windows/irc_window.cpp"
#include "windows/desktop_window.cpp"

//
// Entry point — window objects are heap-allocated to keep the stack lean
//

extern "C" int main()
{
    UIRootImpl *root = new UIRootImpl();
    if (!root || root->HasError())
        return 1;

    UIRootImpl::PlatformWindowOptions opts{};
    opts.initialVisible = true;
    opts.useCustomDPI = true;
    opts.customDPI = 96; // 96, 86, 64, 125 (120% zoom-in)

    // --- Window 1 ---
    HelloWindow *hw = new HelloWindow();
    PlatformWindow *wnd = root->CreateWindow(
        "Hello r2", 0, 0,
        HelloWindow::onEvent, hw,
        &opts, nullptr, nullptr);
    if (!wnd)
        return 1;
    hw->SetWindow(wnd);
    wnd->SetVisible(true);
    root->EnterMainLoop();

    // --- Window 2: Login ---
    if (hw->wantsNext)
    {
        LoginWindow *lw = new LoginWindow();
        PlatformWindow *wnd2 = root->CreateWindow(
            "Login", 0, 0,
            LoginWindow::onEvent, lw,
            &opts, nullptr, nullptr);
        if (wnd2)
        {
            lw->SetWindow(wnd2);
            wnd2->SetVisible(true);
            root->EnterMainLoop();
        }

        // --- Windows 3+: Desktop ↔ Tasks loop ---
        bool showDesktop = lw->wantsDesktop;
#ifdef MEMENTO_BACKEND_R2
        unsigned long heapMark = r2_heap_checkpoint();
#endif
        while (showDesktop)
        {
            showDesktop = false;
#ifdef MEMENTO_BACKEND_R2
            r2_heap_restore(heapMark); // reclaim previous iteration's windows
            resetWallpaperCache();     // dangling after heap restore
#endif
            DesktopWindow *desk = new DesktopWindow();
            PlatformWindow *wnd3 = root->CreateWindow(
                "Desktop", 0, 0,
                DesktopWindow::onEvent, desk,
                &opts, nullptr, nullptr);
            if (!wnd3)
                break;
            desk->SetWindow(wnd3);
            wnd3->SetVisible(true);
            root->EnterMainLoop();

            if (desk->wantsClock)
            {
                ClockWindow *clk = new ClockWindow();
                PlatformWindow *wndCl = root->CreateWindow(
                    "Clock", 0, 0,
                    ClockWindow::onEvent, clk,
                    &opts, nullptr, nullptr);
                if (wndCl)
                {
                    clk->SetWindow(wndCl);
                    wndCl->SetVisible(true);
                    root->EnterMainLoop();
                }
                showDesktop = true;
            }
            else if (desk->wantsShell)
            {
#ifdef MEMENTO_BACKEND_R2
                set_video_mode(0x03);
                unsigned char sh_pid = 0;
                if (run_elf((const unsigned char *)"sh.elf",
                            (const unsigned char *)"sh.elf", &sh_pid))
                {
                    int misses = 0;
                    while (true)
                    {
                        sleep_ms(500);
                        TaskInfo_T tasks[16];
                        long n = list_tasks(tasks, 16);
                        if (n <= 0)
                            continue; // try_lock failed, keep waiting
                        bool alive = false;
                        for (long i = 0; i < n; i++)
                        {
                            // Match by name ("SH.ELF" → name[0]='S', name[1]='H')
                            // and status < 4 (Ready/Running/Idle/Blocked)
                            if (tasks[i].name[0] == 'S' && tasks[i].name[1] == 'H' && tasks[i].status < 4)
                            {
                                alive = true;
                                break;
                            }
                        }
                        if (alive)
                        {
                            misses = 0;
                            continue;
                        }
                        if (++misses >= 3)
                            break; // 3 consecutive misses = shell gone
                    }
                }
                set_video_mode(0x13);
#endif
                showDesktop = true;
            }
            else if (desk->wantsNet)
            {
                NetWindow *nw = new NetWindow();
                PlatformWindow *wndN = root->CreateWindow(
                    "Network", 0, 0,
                    NetWindow::onEvent, nw,
                    &opts, nullptr, nullptr);
                if (wndN)
                {
                    nw->SetWindow(wndN);
                    wndN->SetVisible(true);
                    root->EnterMainLoop();
                }
                showDesktop = true;
            }
            else if (desk->wantsTasks)
            {
                TasksWindow *tw = new TasksWindow();
                PlatformWindow *wnd4 = root->CreateWindow(
                    "Tasks", 0, 0,
                    TasksWindow::onEvent, tw,
                    &opts, nullptr, nullptr);
                if (wnd4)
                {
                    tw->SetWindow(wnd4);
                    wnd4->SetVisible(true);
                    root->EnterMainLoop();
                }
                showDesktop = true;
            }
            else if (desk->wantsMount)
            {
#ifdef MEMENTO_BACKEND_R2
                unsigned long mountMark = r2_heap_checkpoint();
#endif
                bool showMount = true;
                while (showMount)
                {
                    showMount = false;
#ifdef MEMENTO_BACKEND_R2
                    r2_heap_restore(mountMark);
                    resetWallpaperCache();
#endif
                    MountWindow *mw = new MountWindow();
                    PlatformWindow *wnd5 = root->CreateWindow(
                        "Files", 0, 0, MountWindow::onEvent, mw, &opts, nullptr, nullptr);
                    if (wnd5)
                    {
                        mw->SetWindow(wnd5);
                        wnd5->SetVisible(true);
                        root->EnterMainLoop();
                    }
                    if (mw->wantsViewFile)
                    {
                        // Copy path to stack before heap restore frees mw
                        char vpath[128] = {};
                        unsigned int vsz = mw->viewFileSize;
                        for (int i = 0; mw->viewFilePath[i] && i < 127; i++)
                            vpath[i] = mw->viewFilePath[i];
#ifdef MEMENTO_BACKEND_R2
                        r2_heap_restore(mountMark);
#endif
                        FileViewerWindow *fvw = new FileViewerWindow(vpath, vsz);
                        PlatformWindow *wndF = root->CreateWindow(
                            "File", 0, 0, FileViewerWindow::onEvent, fvw, &opts, nullptr, nullptr);
                        if (wndF)
                        {
                            fvw->SetWindow(wndF);
                            wndF->SetVisible(true);
                            root->EnterMainLoop();
                        }
                        showMount = true; // return to file browser after viewing
                    }
                }
                showDesktop = true;
            }
            else if (desk->wantsChat)
            {
                ChatWindow *cw = new ChatWindow();
                PlatformWindow *wndC = root->CreateWindow(
                    "Chat", 0, 0, ChatWindow::onEvent, cw, &opts, nullptr, nullptr);
                if (wndC)
                {
                    cw->SetWindow(wndC);
                    wndC->SetVisible(true);
                    root->EnterMainLoop();
                }
                delete cw;
                showDesktop = true;
            }
            else if (desk->wantsCalc)
            {
                CalculatorWindow *calc = new CalculatorWindow();
                PlatformWindow *wndCa = root->CreateWindow(
                    "Calculator", 0, 0, CalculatorWindow::onEvent, calc, &opts, nullptr, nullptr);
                if (wndCa)
                {
                    calc->SetWindow(wndCa);
                    wndCa->SetVisible(true);
                    root->EnterMainLoop();
                }
                delete calc;
                showDesktop = true;
            }
            else if (desk->wantsIRC)
            {
                IRCWindow *irc = new IRCWindow();
                PlatformWindow *wndI = root->CreateWindow(
                    "IRC", 0, 0, IRCWindow::onEvent, irc, &opts, nullptr, nullptr);
                if (wndI)
                {
                    irc->SetWindow(wndI);
                    wndI->SetVisible(true);
                    root->EnterMainLoop();
                }
                delete irc;
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
