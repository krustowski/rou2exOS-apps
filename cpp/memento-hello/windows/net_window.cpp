//
// Window — Network Status  (ScNetStatus 0x38)
// Shows IP, MAC, driver state and bound TCP port registry.
//

class NetWindow
{
public:
    static void onEvent(void *instance, struct PlatformWindowInterfaceInputEvent *data)
    {
        reinterpret_cast<NetWindow *>(instance)->onEvent_(data);
    }
    void SetWindow(PlatformWindow *w) { wnd = w; }

private:
    PlatformWindow *wnd = nullptr;
    PlatformColor *dark = nullptr;
    PlatformColor *light = nullptr;
    PlatformFont *font = nullptr;

    static char hexNibble(unsigned char n) { return n < 10 ? '0' + n : 'a' + (n - 10); }

    static void byteToStr(unsigned char b, char *out)
    {
        if (b >= 100)
        {
            out[0] = '0' + b / 100;
            out[1] = '0' + (b / 10) % 10;
            out[2] = '0' + b % 10;
            out[3] = 0;
        }
        else if (b >= 10)
        {
            out[0] = '0' + b / 10;
            out[1] = '0' + b % 10;
            out[2] = 0;
        }
        else
        {
            out[0] = '0' + b;
            out[1] = 0;
        }
    }

    static void u16ToStr(unsigned short n, char *out)
    {
        if (!n)
        {
            out[0] = '0';
            out[1] = 0;
            return;
        }
        char t[6];
        int i = 0;
        while (n)
        {
            t[i++] = '0' + n % 10;
            n /= 10;
        }
        for (int j = 0; j < i; j++)
            out[j] = t[i - 1 - j];
        out[i] = 0;
    }

    // "a.b.c.d\0" — caller supplies buf[16]
    static void ipToStr(const unsigned char ip[4], char *buf)
    {
        int pos = 0;
        for (int i = 0; i < 4; i++)
        {
            char tmp[4];
            byteToStr(ip[i], tmp);
            for (int j = 0; tmp[j]; j++)
                buf[pos++] = tmp[j];
            if (i < 3)
                buf[pos++] = '.';
        }
        buf[pos] = 0;
    }

    // "aa:bb:cc:dd:ee:ff\0" — caller supplies buf[18]
    static void macToStr(const unsigned char mac[6], char *buf)
    {
        for (int i = 0; i < 6; i++)
        {
            buf[i * 3] = hexNibble(mac[i] >> 4);
            buf[i * 3 + 1] = hexNibble(mac[i] & 0xF);
            buf[i * 3 + 2] = (i < 5) ? ':' : 0;
        }
        buf[17] = 0;
    }

    void onEvent_(struct PlatformWindowInterfaceInputEvent *data)
    {
        if (data->type == PlatformWindowInputEventType::OnPaint)
        {
            OnPaint(data->Data.OnPaint.ctx, data->Data.OnPaint.target);
            return;
        }
        if (data->type == PlatformWindowInputEventType::OnMouseClick)
        {
            if (data->Data.OnMouseClick.state == PlatformWindowButtonState::Pressed)
                wnd->Close();
            return;
        }
        if (data->type != PlatformWindowInputEventType::OnKeyEvent)
            return;
        auto *key = data->Data.OnKeyEvent.key;
        if (!key->isKeyDown)
            return;
        if (key->isEscape || key->isEnter)
            wnd->Close();
    }

    void OnPaint(PlatformDrawingContext *dc, PlatformBitmap *target)
    {
        if (!target)
            return;
        if (!dark)
            dark = dc->CreateColor(0xFF0A0A20, nullptr, nullptr);
        if (!light)
            light = dc->CreateColor(0xFFE0E0FF, nullptr, nullptr);
        if (!font)
            font = dc->CreateFont(12, nullptr, false, false, false, nullptr, nullptr);
        if (!dark || !light || !font)
            return;

        NetStatus_T ns{};
        get_net_status(&ns);
        SysInfo_T si{};
        read_sysinfo(&si);

        Coord W = target->GetWidth(), H = target->GetHeight();
        target->FillRect(0, 0, W, H, dark, false);
        drawWallpaper(dc, target);
        target->FillRect(0, H - 14, W, 1, dark, false);
        target->FillRect(0, H - 13, W, 13, light, false);
        target->FillRect(5, 8, 310, 175, dark, false);
        target->FillRect(7, 10, 306, 171, light, false);
        target->FillRect(7, 24, 306, 1, dark, false); // title sep

        PlatformDrawTextOptions opts{};
        opts.font = font;
        opts.foreground = dark;
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign = PlatformAlign::Middle;
        target->DrawText(7, 10, 280, 14, "Network", &opts, false);
        target->DrawText(0, H - 13, W, 13, "Net  -  r2", &opts, false);

        // Section: Interface
        opts.horizontalAlign = PlatformAlign::Begin;

        // IP row — from sysinfo (set by ETH driver via ScSysInfo 0x02)
        char ipbuf[16];
        ipToStr(si.ip_addr, ipbuf);
        target->DrawText(10, 28, 36, 12, "IP:", &opts, false);
        target->DrawText(50, 28, 250, 12, (const mchar *)ipbuf, &opts, false);

        // MAC row
        char macbuf[18];
        macToStr(ns.mac, macbuf);
        target->DrawText(10, 42, 36, 12, "MAC:", &opts, false);
        target->DrawText(50, 42, 250, 12, (const mchar *)macbuf, &opts, false);

        // Driver row
        target->DrawText(10, 56, 60, 12, "Driver:", &opts, false);
        target->DrawText(76, 56, 220, 12,
                         ns.drv_active ? "Active" : "Inactive", &opts, false);

        // Separator before port table
        target->FillRect(7, 72, 306, 1, dark, false);

        // Ports section header
        target->DrawText(10, 75, 70, 12, "TCP ports:", &opts, false);

        if (ns.n_ports == 0)
        {
            target->DrawText(90, 75, 210, 12, "(none registered)", &opts, false);
        }
        else
        {
            // Render ports as a space-separated run, wrapping every 8 per row
            static const int COLS = 8;
            for (int i = 0; i < ns.n_ports && i < 16; i++)
            {
                int row = i / COLS, col = i % COLS;
                Coord ry = 75 + row * 13;
                Coord rx = (col == 0) ? 90 : 90 + col * 36;
                if (col == 0 && row > 0)
                {
                    // new row label blank
                    rx = 10;
                    // shift the column positions on rows > 0
                    rx = 10 + (i % COLS) * 36;
                }
                char pbuf[6];
                u16ToStr(ns.ports[i], pbuf);
                target->DrawText(rx, ry, 35, 12, (const mchar *)pbuf, &opts, false);
            }
        }

        // Back button
        target->FillRect(7, 158, 306, 1, dark, false);
        target->FillRect(120, 161, 80, 13, dark, false);
        target->FillRect(121, 162, 78, 11, light, false);
        opts.foreground = dark;
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign = PlatformAlign::Middle;
        target->DrawText(120, 161, 80, 13, "Back", &opts, false);
    }
};
