//
// Window 4 — Task Manager  (live data via ScListTasks 0x2F)
//

class TasksWindow
{
public:
    static void onEvent(void *instance, struct PlatformWindowInterfaceInputEvent *data)
    {
        reinterpret_cast<TasksWindow *>(instance)->onEvent_(data);
    }
    void SetWindow(PlatformWindow *w) { wnd = w; }

private:
    PlatformWindow *wnd = nullptr;
    PlatformColor *dark = nullptr;
    PlatformColor *light = nullptr;
    PlatformFont *font = nullptr;
    int sel = 0;
    int nLive = 0; // last known task count; key handlers use it

    static const char *statusStr(unsigned char s)
    {
        if (s == 0)
            return "Ready";
        if (s == 1)
            return "Running";
        if (s == 2)
            return "Idle";
        if (s == 3)
            return "Blocked";
        if (s == 4)
            return "Crashed";
        if (s == 5)
            return "Dead";
        return "?";
    }
    static const char *modeStr(unsigned char m) { return m ? "User" : "Kernel"; }

    static void pidStr(unsigned char n, char *out)
    {
        if (n >= 100)
        {
            out[0] = '0' + n / 100;
            out[1] = '0' + (n / 10) % 10;
            out[2] = '0' + n % 10;
            out[3] = 0;
        }
        else if (n >= 10)
        {
            out[0] = '0' + n / 10;
            out[1] = '0' + n % 10;
            out[2] = 0;
        }
        else
        {
            out[0] = '0' + n;
            out[1] = 0;
        }
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
            if (data->Data.OnMouseClick.state != PlatformWindowButtonState::Pressed)
                return;
            Coord mx = data->Data.OnMouseClick.mouseX;
            Coord my = data->Data.OnMouseClick.mouseY;
            // Back button at y=166, h=13
            if (my >= 166 && my < 179 && mx >= 120 && mx < 200)
            {
                wnd->Close();
                return;
            }
            // Task rows start at y=40, each 12px tall; hit = select row
            for (int i = 0; i < nLive; i++)
            {
                if (my >= 40 + i * 12 && my < 40 + i * 12 + 11)
                {
                    sel = i;
                    wnd->Repaint();
                    break;
                }
            }
            return;
        }
        if (data->type != PlatformWindowInputEventType::OnKeyEvent)
            return;

        auto *key = data->Data.OnKeyEvent.key;

        if (!key->isKeyDown)
            return;
        if (key->isEscape)
        {
            wnd->Close();
            return;
        }
        if (key->isArrowUp)
        {
            if (sel > 0)
            {
                sel--;
                wnd->Repaint();
            }
            return;
        }
        if (key->isArrowDown)
        {
            if (sel < nLive)
            {
                sel++;
                wnd->Repaint();
            }
            return;
        }
        if (key->isEnter && sel == nLive)
        {
            wnd->Close();
        }
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

        // Fetch live task list (max 10, 20 bytes each = 200 bytes on stack)
        TaskInfo_T buf[10];
        int n = (int)list_tasks(buf, 10);
        if (n < 0)
            n = 0;
        nLive = n;
        if (sel > nLive)
            sel = nLive;

        Coord W = target->GetWidth();
        Coord H = target->GetHeight();

        target->FillRect(0, 0, W, H, dark, false);
        drawWallpaper(dc, target);
        target->FillRect(0, H - 14, W, 1, dark, false);
        target->FillRect(0, H - 13, W, 13, light, false);

        // Window chrome — tall enough for 10 rows + header + back button
        // Outer y=8..182, inner y=10..180
        target->FillRect(5, 8, 310, 175, dark, false);
        target->FillRect(7, 10, 306, 171, light, false);
        target->FillRect(7, 24, 306, 1, dark, false); // title separator

        PlatformDrawTextOptions opts{};
        opts.font = font;
        opts.foreground = dark;
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign = PlatformAlign::Middle;

        target->DrawText(7, 10, 280, 14, "Task Manager", &opts, false);
        target->DrawText(0, H - 13, W, 13, "Tasks  -  r2", &opts, false);

        // Column headers  (x: PID=10 Name=44 Mode=198 Status=248)
        opts.horizontalAlign = PlatformAlign::Begin;
        target->DrawText(10, 26, 30, 12, "PID", &opts, false);
        target->DrawText(44, 26, 150, 12, "Name", &opts, false);
        target->DrawText(198, 26, 46, 12, "Mode", &opts, false);
        target->DrawText(248, 26, 58, 12, "Status", &opts, false);
        target->FillRect(7, 38, 306, 1, dark, false);

        // Task rows — 12 px each, starting at y=40
        for (int i = 0; i < n; i++)
        {
            Coord ry = 40 + i * 12;
            if (sel == i)
            {
                target->FillRect(8, ry, 304, 11, dark, false);
                opts.foreground = light;
            }
            else
            {
                opts.foreground = dark;
            }
            char pidbuf[4];
            pidStr(buf[i].id, pidbuf);
            char namebuf[17];
            for (int j = 0; j < 16; j++)
                namebuf[j] = (char)buf[i].name[j];

            namebuf[16] = 0;

            opts.horizontalAlign = PlatformAlign::Begin;
            target->DrawText(10, ry, 30, 11, (const mchar *)pidbuf, &opts, false);
            target->DrawText(44, ry, 150, 11, (const mchar *)namebuf, &opts, false);
            target->DrawText(198, ry, 46, 11, (const mchar *)modeStr(buf[i].mode), &opts, false);
            target->DrawText(248, ry, 58, 11, (const mchar *)statusStr(buf[i].status), &opts, false);
        }

        // Separator + Back button
        target->FillRect(7, 163, 306, 1, dark, false);
        bool backFocused = (sel == nLive);
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign = PlatformAlign::Middle;

        if (backFocused)
        {
            target->FillRect(120, 166, 80, 13, dark, false);
            opts.foreground = light;
        }
        else
        {
            target->FillRect(120, 166, 80, 13, dark, false);
            target->FillRect(121, 167, 78, 11, light, false);
            opts.foreground = dark;
        }

        target->DrawText(120, 166, 80, 13, "Back", &opts, false);
    }
};
