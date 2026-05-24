//
// Window 6 — Desktop launcher
//

class DesktopWindow
{
public:
    static void onEvent(void *instance, struct PlatformWindowInterfaceInputEvent *data)
    {
        reinterpret_cast<DesktopWindow *>(instance)->onEvent_(data);
    }

    void SetWindow(PlatformWindow *w) { wnd = w; }
    bool wantsClock = false;
    bool wantsTasks = false;
    bool wantsMount = false;
    bool wantsNet = false;
    bool wantsShell = false;
    bool wantsChat = false;
    bool wantsCalc = false;
    bool wantsIRC = false;

private:
    PlatformWindow *wnd = nullptr;
    PlatformColor *dark = nullptr;
    PlatformColor *light = nullptr;
    PlatformFont *font = nullptr;
    int sel = 0; // 0=Clock 1=Shell 2=Net 3=Mount 4=Tasks 5=Chat 6=Calc 7=IRC

    // Row 1: 5 icons at 42-px spacing; Row 2: 2 icons (Chat, Calc).
    // Frame is FW=230 wide, centered on a 320px canvas: FX=(320-230)/2=45.
    static const int BSIZ = 29;
    static const int FW = 230; // dialog frame width
    static const int FX = 45;  // frame left edge (centered)
    static const int IX0 = 61, IX1 = 103, IX2 = 145, IX3 = 187, IX4 = 229;
    static const int IY = 43;   // row 1 icon top (below 16 px title bar)
    static const int LY = 75;   // row 1 label top
    static const int IY2 = 91;  // row 2 icon top
    static const int LY2 = 123; // row 2 label top
    static const int LW = 44;
    static const int LH = 12;

    PlatformBitmap *bmpClock = nullptr;
    PlatformBitmap *bmpShell = nullptr;
    PlatformBitmap *bmpNet = nullptr;
    PlatformBitmap *bmpMount = nullptr;
    PlatformBitmap *bmpTasks = nullptr;
    PlatformBitmap *bmpChat = nullptr;
    PlatformBitmap *bmpCalc = nullptr;
    PlatformBitmap *bmpIRC = nullptr;

    void MakeBitmaps(PlatformDrawingContext *dc)
    {
        // Clock: circular face, corner roundoff, tick marks, hands
        if (!bmpClock)
        {
            bmpClock = dc->CreateBitmap(Coord(32), Coord(32), nullptr, nullptr);
            if (bmpClock)
            {
                bmpClock->FillRectD(Dim(0), Dim(0), Dim(29), Dim(29), dark);  // bg
                bmpClock->FillRectD(Dim(3), Dim(3), Dim(23), Dim(23), light); // face
                bmpClock->FillRectD(Dim(3), Dim(3), Dim(3), Dim(3), dark);    // corner TL
                bmpClock->FillRectD(Dim(23), Dim(3), Dim(3), Dim(3), dark);   // corner TR
                bmpClock->FillRectD(Dim(3), Dim(23), Dim(3), Dim(3), dark);   // corner BL
                bmpClock->FillRectD(Dim(23), Dim(23), Dim(3), Dim(3), dark);  // corner BR
                bmpClock->FillRectD(Dim(12), Dim(4), Dim(5), Dim(2), dark);   // 12 tick
                bmpClock->FillRectD(Dim(23), Dim(12), Dim(2), Dim(5), dark);  // 3  tick
                bmpClock->FillRectD(Dim(12), Dim(23), Dim(5), Dim(2), dark);  // 6  tick
                bmpClock->FillRectD(Dim(4), Dim(12), Dim(2), Dim(5), dark);   // 9  tick
                bmpClock->FillRectD(Dim(13), Dim(7), Dim(2), Dim(7), dark);   // hour hand
                bmpClock->FillRectD(Dim(14), Dim(13), Dim(7), Dim(2), dark);  // min  hand
                bmpClock->FillRectD(Dim(13), Dim(13), Dim(2), Dim(2), dark);  // pivot
            }
        }
        // Shell: terminal window with title bar dots and ">_" prompt
        if (!bmpShell)
        {
            bmpShell = dc->CreateBitmap(Coord(32), Coord(32), nullptr, nullptr);
            if (bmpShell)
            {
                PlatformDrawTextOptions to{};
                to.font = font;
                to.foreground = light;
                to.horizontalAlign = PlatformAlign::Begin;
                to.verticalAlign = PlatformAlign::Begin;
                bmpShell->FillRectD(Dim(0), Dim(0), Dim(29), Dim(29), dark); // bg
                bmpShell->FillRectD(Dim(2), Dim(2), Dim(25), Dim(5), light); // title bar
                bmpShell->FillRectD(Dim(4), Dim(3), Dim(3), Dim(3), dark);   // dot 1
                bmpShell->FillRectD(Dim(9), Dim(3), Dim(3), Dim(3), dark);   // dot 2
                bmpShell->FillRectD(Dim(14), Dim(3), Dim(3), Dim(3), dark);  // dot 3
                bmpShell->DrawTextD(Dim(3), Dim(9), Dim(24), Dim(16), ">_", &to);
            }
        }
        // Net: parabolic dish (opens right) + signal glyphs << / >>
        if (!bmpNet)
        {
            bmpNet = dc->CreateBitmap(Coord(32), Coord(32), nullptr, nullptr);
            if (bmpNet)
            {
                PlatformDrawTextOptions to{};
                to.font = font;
                to.foreground = light;
                to.horizontalAlign = PlatformAlign::Begin;
                to.verticalAlign = PlatformAlign::Begin;
                bmpNet->FillRectD(Dim(0), Dim(0), Dim(29), Dim(29), dark);
                bmpNet->FillRectD(Dim(10), Dim(3), Dim(4), Dim(2), light);  // top arm
                bmpNet->FillRectD(Dim(7), Dim(5), Dim(4), Dim(2), light);   // curve
                bmpNet->FillRectD(Dim(5), Dim(7), Dim(3), Dim(2), light);   // curve
                bmpNet->FillRectD(Dim(3), Dim(9), Dim(3), Dim(2), light);   // curve
                bmpNet->FillRectD(Dim(2), Dim(11), Dim(3), Dim(4), light);  // apex
                bmpNet->FillRectD(Dim(3), Dim(15), Dim(3), Dim(2), light);  // curve
                bmpNet->FillRectD(Dim(5), Dim(17), Dim(3), Dim(2), light);  // curve
                bmpNet->FillRectD(Dim(7), Dim(19), Dim(4), Dim(2), light);  // curve
                bmpNet->FillRectD(Dim(10), Dim(21), Dim(4), Dim(2), light); // bottom arm
                bmpNet->FillRectD(Dim(9), Dim(23), Dim(5), Dim(2), light);  // base top
                bmpNet->FillRectD(Dim(7), Dim(25), Dim(7), Dim(2), light);  // base mid
                bmpNet->FillRectD(Dim(5), Dim(27), Dim(10), Dim(2), light); // base foot
                bmpNet->DrawTextD(Dim(15), Dim(3), Dim(12), Dim(10), "<<", &to);
                bmpNet->DrawTextD(Dim(15), Dim(13), Dim(12), Dim(10), ">>", &to);
            }
        }
        // Mount: three stacked bars with left-side label dot
        if (!bmpMount)
        {
            bmpMount = dc->CreateBitmap(Coord(32), Coord(32), nullptr, nullptr);
            if (bmpMount)
            {
                bmpMount->FillRectD(Dim(0), Dim(0), Dim(29), Dim(29), dark);
                bmpMount->FillRectD(Dim(3), Dim(4), Dim(23), Dim(5), light);  // bar 1
                bmpMount->FillRectD(Dim(3), Dim(12), Dim(23), Dim(5), light); // bar 2
                bmpMount->FillRectD(Dim(3), Dim(20), Dim(23), Dim(5), light); // bar 3
                bmpMount->FillRectD(Dim(5), Dim(6), Dim(4), Dim(2), dark);    // dot 1
                bmpMount->FillRectD(Dim(5), Dim(14), Dim(4), Dim(2), dark);   // dot 2
                bmpMount->FillRectD(Dim(5), Dim(22), Dim(4), Dim(2), dark);   // dot 3
            }
        }
        // Tasks: bar-graph with 4 bars of varying height over a baseline
        if (!bmpTasks)
        {
            bmpTasks = dc->CreateBitmap(Coord(32), Coord(32), nullptr, nullptr);
            if (bmpTasks)
            {
                bmpTasks->FillRectD(Dim(0), Dim(0), Dim(29), Dim(29), dark);
                bmpTasks->FillRectD(Dim(3), Dim(18), Dim(4), Dim(7), light);   // bar 1
                bmpTasks->FillRectD(Dim(9), Dim(11), Dim(4), Dim(14), light);  // bar 2
                bmpTasks->FillRectD(Dim(15), Dim(14), Dim(4), Dim(11), light); // bar 3
                bmpTasks->FillRectD(Dim(22), Dim(7), Dim(4), Dim(18), light);  // bar 4
                bmpTasks->FillRectD(Dim(3), Dim(25), Dim(23), Dim(2), light);  // baseline
            }
        }
        // Chat: speech bubble with tail and three dots
        if (!bmpChat)
        {
            bmpChat = dc->CreateBitmap(Coord(32), Coord(32), nullptr, nullptr);
            if (bmpChat)
            {
                bmpChat->FillRectD(Dim(0), Dim(0), Dim(29), Dim(29), dark);  // bg
                bmpChat->FillRectD(Dim(2), Dim(3), Dim(23), Dim(14), light); // bubble body
                bmpChat->FillRectD(Dim(2), Dim(17), Dim(7), Dim(3), light);  // tail top
                bmpChat->FillRectD(Dim(2), Dim(20), Dim(5), Dim(2), light);  // tail mid
                bmpChat->FillRectD(Dim(2), Dim(22), Dim(3), Dim(2), light);  // tail tip
                bmpChat->FillRectD(Dim(6), Dim(9), Dim(3), Dim(3), dark);    // dot 1
                bmpChat->FillRectD(Dim(12), Dim(9), Dim(3), Dim(3), dark);   // dot 2
                bmpChat->FillRectD(Dim(18), Dim(9), Dim(3), Dim(3), dark);   // dot 3
            }
        }
        // Calc: calculator body with display strip and 4×3 key grid
        if (!bmpCalc)
        {
            bmpCalc = dc->CreateBitmap(Coord(32), Coord(32), nullptr, nullptr);
            if (bmpCalc)
            {
                bmpCalc->FillRectD(Dim(0), Dim(0), Dim(29), Dim(29), dark);  // bg
                bmpCalc->FillRectD(Dim(3), Dim(2), Dim(23), Dim(29), light); // body
                bmpCalc->FillRectD(Dim(4), Dim(3), Dim(21), Dim(6), dark);   // display
                // 4×3 key grid
                for (int r = 0; r < 4; r++)
                    for (int c = 0; c < 3; c++)
                        bmpCalc->FillRectD(Dim(5 + c * 7), Dim(12 + r * 5), Dim(5), Dim(3), dark);
            }
        }
        // IRC: # symbol (two horizontal bars crossing two vertical bars)
        if (!bmpIRC)
        {
            bmpIRC = dc->CreateBitmap(Coord(32), Coord(32), nullptr, nullptr);
            if (bmpIRC)
            {
                bmpIRC->FillRectD(Dim(0), Dim(0), Dim(29), Dim(29), dark);  // bg
                bmpIRC->FillRectD(Dim(9), Dim(4), Dim(3), Dim(21), light);  // left  vert bar
                bmpIRC->FillRectD(Dim(17), Dim(4), Dim(3), Dim(21), light); // right vert bar
                bmpIRC->FillRectD(Dim(5), Dim(9), Dim(19), Dim(3), light);  // top   horiz bar
                bmpIRC->FillRectD(Dim(5), Dim(17), Dim(19), Dim(3), light); // bot   horiz bar
            }
        }
    }

    void BlitIcon(PlatformBitmap *t, PlatformBitmap *bm, int ix, int iy, bool s)
    {
        if (s)
            t->FillRectD(Dim(ix - 2), Dim(iy - 2), Dim(BSIZ + 4), Dim(BSIZ + 4), dark);
        if (bm)
            t->CopyBitmapD(Dim(ix), Dim(iy), Dim(BSIZ), Dim(BSIZ),
                           bm, Dim(0), Dim(0), Dim(BSIZ), Dim(BSIZ), false, 255);
    }

    void launchSel(int s)
    {
        if (s == 0)
            wantsClock = true;
        if (s == 1)
            wantsShell = true;
        if (s == 2)
            wantsNet = true;
        if (s == 3)
            wantsMount = true;
        if (s == 4)
            wantsTasks = true;
        if (s == 5)
            wantsChat = true;
        if (s == 6)
            wantsCalc = true;
        if (s == 7)
            wantsIRC = true;
        wnd->Close();
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
            // Row 1: Clock..Tasks at IX0..IX4; Row 2: Chat at IX0, Calc at IX1
            const int IXs[5] = {IX0, IX1, IX2, IX3, IX4};
            if (my >= IY && my < IY + BSIZ)
            {
                for (int i = 0; i < 5; i++)
                {
                    if (mx >= IXs[i] && mx < IXs[i] + BSIZ)
                    {
                        launchSel(i);
                        return;
                    }
                }
            }
            if (my >= IY2 && my < IY2 + BSIZ)
            {
                if (mx >= IX0 && mx < IX0 + BSIZ)
                {
                    launchSel(5);
                    return;
                }
                if (mx >= IX1 && mx < IX1 + BSIZ)
                {
                    launchSel(6);
                    return;
                }
                if (mx >= IX2 && mx < IX2 + BSIZ)
                {
                    launchSel(7);
                    return;
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
        if (key->isArrowLeft || key->isArrowUp)
        {
            sel = (sel + 7) % 8;
            wnd->Repaint();
            return;
        }
        if (key->isArrowRight || key->isArrowDown)
        {
            sel = (sel + 1) % 8;
            wnd->Repaint();
            return;
        }
        if (key->isEnter)
        {
            launchSel(sel);
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
        MakeBitmaps(dc);

        Coord W = target->GetWidth();
        Coord H = target->GetHeight();

        target->FillRect(0, 0, W, H, dark, false);
        drawWallpaper(dc, target);

        // Taskbar
        target->FillRect(0, H - 14, W, 1, dark, false);
        target->FillRect(0, H - 13, W, 13, light, false);

        // Dialog frame — centered, width FW, left edge at FX
        target->FillRect(FX, 22, FW, 118, dark, false);
        target->FillRect(FX + 2, 24, FW - 4, 114, light, false);
        target->FillRect(FX + 2, 40, FW - 4, 1, dark, false); // title separator

        PlatformDrawTextOptions opts{};
        opts.font = font;
        opts.foreground = dark;
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign = PlatformAlign::Middle;

        target->DrawText(FX + 2, 24, FW - 4, 16, "Desktop", &opts, false);
        target->DrawText(0, H - 13, W, 13, "Desktop  -  r2", &opts, false);

        // Row 1 icons (Clock, Shell, Net, Mount, Tasks)
        BlitIcon(target, bmpClock, IX0, IY, sel == 0);
        BlitIcon(target, bmpShell, IX1, IY, sel == 1);
        BlitIcon(target, bmpNet, IX2, IY, sel == 2);
        BlitIcon(target, bmpMount, IX3, IY, sel == 3);
        BlitIcon(target, bmpTasks, IX4, IY, sel == 4);
        // Row 2 icons (Chat, Calc, IRC)
        BlitIcon(target, bmpChat, IX0, IY2, sel == 5);
        BlitIcon(target, bmpCalc, IX1, IY2, sel == 6);
        BlitIcon(target, bmpIRC, IX2, IY2, sel == 7);

        // Labels
        PlatformDrawTextOptions lo{};
        lo.font = font;
        lo.foreground = dark;
        lo.horizontalAlign = PlatformAlign::Middle;
        lo.verticalAlign = PlatformAlign::Middle;
        const int loff = (LW - BSIZ) / 2; // 7 px: centres LW box on BSIZ icon
        target->DrawTextD(Dim(IX0 - loff), Dim(LY), Dim(LW), Dim(LH), "Clock", &lo);
        target->DrawTextD(Dim(IX1 - loff), Dim(LY), Dim(LW), Dim(LH), "Shell", &lo);
        target->DrawTextD(Dim(IX2 - loff), Dim(LY), Dim(LW), Dim(LH), "Net", &lo);
        target->DrawTextD(Dim(IX3 - loff), Dim(LY), Dim(LW), Dim(LH), "Mount", &lo);
        target->DrawTextD(Dim(IX4 - loff), Dim(LY), Dim(LW), Dim(LH), "Tasks", &lo);
        target->DrawTextD(Dim(IX0 - loff), Dim(LY2), Dim(LW), Dim(LH), "Chat", &lo);
        target->DrawTextD(Dim(IX1 - loff), Dim(LY2), Dim(LW), Dim(LH), "Calc", &lo);
        target->DrawTextD(Dim(IX2 - loff), Dim(LY2), Dim(LW), Dim(LH), "IRC", &lo);
    }
};
