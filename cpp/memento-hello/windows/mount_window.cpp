//
// Window — File Browser  (ScListMounts 0x2C + ScListDirPath 0x2D)
// Top level shows mount points; Enter drills into one; Backspace/[..] returns.
//

class MountWindow
{
public:
    MountWindow()
    {
        currentPath[0] = 0;
        mountRoot[0] = 0;
        viewFilePath[0] = 0;
    }

    bool wantsViewFile = false;
    char viewFilePath[128];
    unsigned int viewFileSize = 0;

    static void onEvent(void *instance, struct PlatformWindowInterfaceInputEvent *data)
    {
        reinterpret_cast<MountWindow *>(instance)->onEvent_(data);
    }
    void SetWindow(PlatformWindow *w) { wnd = w; }

private:
    PlatformWindow *wnd = nullptr;
    PlatformColor *dark = nullptr;
    PlatformColor *light = nullptr;
    PlatformFont *font = nullptr;
    int sel = 0;
    int scrollTop = 0;
    bool atMounts = true; // true = mount list, false = dir listing
    char currentPath[128];
    char mountRoot[33]; // path of the mount we entered

    MountInfo_T mounts[8];
    int nMounts = 0;
    VfsDirEntry_T entries[64];
    int nEntries = 0;

    static const int VIS = 10;

    static bool streq(const char *a, const char *b)
    {
        while (*a && *b && *a == *b)
        {
            a++;
            b++;
        }
        return *a == 0 && *b == 0;
    }
    int plen()
    {
        int i = 0;
        while (currentPath[i])
            i++;
        return i;
    }

    static const char *fsType(unsigned char t)
    {
        if (t == 1)
            return "rootfs";
        if (t == 2)
            return "fat12";
        if (t == 3)
            return "iso9660";
        return "none";
    }

    // Enter a mount — copy its null-terminated path into currentPath & mountRoot
    void enterMount(int mi)
    {
        if (mi < 0 || mi >= nMounts)
            return;
        int nl = mounts[mi].path_len < 32 ? mounts[mi].path_len : 32;
        for (int i = 0; i < nl; i++)
            currentPath[i] = mountRoot[i] = (char)mounts[mi].path[i];
        currentPath[nl] = mountRoot[nl] = 0;
        if (nl == 0)
        {
            currentPath[0] = mountRoot[0] = '/';
            currentPath[1] = mountRoot[1] = 0;
        }
        atMounts = false;
        sel = 0;
        scrollTop = 0;
    }

    // Go up: return to mount list if at mount root, else strip last path component
    void goUp()
    {
        if (streq(currentPath, mountRoot))
        {
            atMounts = true;
            sel = 0;
            scrollTop = 0;
            return;
        }
        int len = plen(), i = len - 1;
        while (i > 0 && currentPath[i] != '/')
            i--;
        if (i == 0)
            currentPath[1] = 0;
        else
            currentPath[i] = 0;
        sel = 0;
        scrollTop = 0;
    }

    // Navigate into a subdirectory entry
    void goInto(int ei)
    {
        if (ei < 0 || ei >= nEntries || !entries[ei].is_dir)
            return;
        int cl = plen();
        int nl = entries[ei].name_len < 32 ? entries[ei].name_len : 32;
        if (cl + 1 + nl >= 127)
            return;
        if (cl == 1)
        {
            for (int i = 0; i < nl; i++)
                currentPath[1 + i] = (char)entries[ei].name[i];
            currentPath[1 + nl] = 0;
        }
        else
        {
            currentPath[cl] = '/';
            for (int i = 0; i < nl; i++)
                currentPath[cl + 1 + i] = (char)entries[ei].name[i];
            currentPath[cl + 1 + nl] = 0;
        }
        sel = 0;
        scrollTop = 0;
    }

    static void u32str(unsigned int n, char *out)
    {
        if (!n)
        {
            out[0] = '0';
            out[1] = 0;
            return;
        }
        char t[10];
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
            // Back button shared between both views
            if (my >= 161 && my < 174 && mx >= 120 && mx < 200)
            {
                wnd->Close();
                return;
            }
            if (atMounts)
            {
                for (int i = 0; i < nMounts; i++)
                {
                    if (my >= 38 + i * 12 && my < 38 + i * 12 + 11)
                    {
                        enterMount(i);
                        wnd->Repaint();
                        return;
                    }
                }
            }
            else
            {
                // [..] row at vi=0, entries at vi=1..nEntries; rows start at y=38
                int listItems = 1 + nEntries;
                for (int row = 0; row < VIS; row++)
                {
                    int vi = scrollTop + row;
                    if (vi >= listItems)
                        break;
                    if (my >= 38 + row * 12 && my < 38 + row * 12 + 11)
                    {
                        if (vi == 0)
                        {
                            goUp();
                            wnd->Repaint();
                        }
                        else
                        {
                            int ei = vi - 1;
                            if (entries[ei].is_dir)
                            {
                                goInto(ei);
                                wnd->Repaint();
                            }
                            else
                            {
                                int cl = plen();
                                int nl = entries[ei].name_len < 32 ? entries[ei].name_len : 32;
                                int p = 0;
                                for (int i = 0; i < cl; i++)
                                    viewFilePath[p++] = currentPath[i];
                                if (cl > 1)
                                    viewFilePath[p++] = '/';
                                for (int i = 0; i < nl; i++)
                                    viewFilePath[p++] = (char)entries[ei].name[i];
                                viewFilePath[p] = 0;
                                viewFileSize = entries[ei].size;
                                wantsViewFile = true;
                                wnd->Close();
                            }
                        }
                        return;
                    }
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

        if (atMounts)
        {
            // listItems = nMounts; Back at sel==nMounts
            if (key->isArrowUp)
            {
                if (sel > 0)
                {
                    sel--;
                    if (sel < scrollTop)
                        scrollTop = sel;
                }
                wnd->Repaint();
                return;
            }
            if (key->isArrowDown)
            {
                if (sel < nMounts)
                {
                    sel++;
                } // nMounts = Back index
                wnd->Repaint();
                return;
            }
            if (key->isEnter)
            {
                if (sel == nMounts)
                {
                    wnd->Close();
                    return;
                }
                enterMount(sel);
                wnd->Repaint();
            }
        }
        else
        {
            // listItems = 1 + nEntries ([..] + entries); Back at sel==1+nEntries
            int listItems = 1 + nEntries;
            if (key->isBackspace)
            {
                goUp();
                wnd->Repaint();
                return;
            }
            if (key->isArrowUp)
            {
                if (sel > 0)
                {
                    sel--;
                    if (sel < scrollTop)
                        scrollTop = sel;
                }
                wnd->Repaint();
                return;
            }
            if (key->isArrowDown)
            {
                if (sel < listItems)
                {
                    sel++;
                    if (sel < listItems && sel >= scrollTop + VIS)
                        scrollTop = sel - VIS + 1;
                }
                wnd->Repaint();
                return;
            }
            if (key->isEnter)
            {
                if (sel == listItems)
                {
                    wnd->Close();
                    return;
                }
                if (sel == 0)
                {
                    goUp();
                    wnd->Repaint();
                    return;
                }
                int ei = sel - 1;
                if (entries[ei].is_dir)
                {
                    goInto(ei);
                    wnd->Repaint();
                }
                else
                {
                    // Build full path for read_file: currentPath + "/" + name
                    int cl = plen();
                    int nl = entries[ei].name_len < 32 ? entries[ei].name_len : 32;
                    int p = 0;
                    for (int i = 0; i < cl; i++)
                        viewFilePath[p++] = currentPath[i];
                    if (cl > 1)
                        viewFilePath[p++] = '/'; // avoid "//" at root
                    for (int i = 0; i < nl; i++)
                        viewFilePath[p++] = (char)entries[ei].name[i];
                    viewFilePath[p] = 0;
                    viewFileSize = entries[ei].size;
                    wantsViewFile = true;
                    wnd->Close();
                }
            }
        }
    }

    void drawChrome(PlatformDrawingContext *dc, PlatformBitmap *target,
                    const mchar *title, const mchar *pathLine,
                    PlatformDrawTextOptions &opts, Coord W, Coord H)
    {
        target->FillRect(0, 0, W, H, dark, false);
        drawWallpaper(dc, target);
        target->FillRect(0, H - 14, W, 1, dark, false);
        target->FillRect(0, H - 13, W, 13, light, false);
        target->FillRect(5, 8, 310, 175, dark, false);
        target->FillRect(7, 10, 306, 171, light, false);
        target->FillRect(7, 24, 306, 1, dark, false);
        target->FillRect(7, 37, 306, 1, dark, false);
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.foreground = dark;
        target->DrawText(7, 10, 280, 14, title, &opts, false);
        target->DrawText(0, H - 13, W, 13, "Files  -  r2", &opts, false);
        opts.horizontalAlign = PlatformAlign::Begin;
        target->DrawText(10, 25, 290, 12, pathLine, &opts, false);
    }

    void drawBack(PlatformBitmap *target, bool focused, PlatformDrawTextOptions &opts)
    {
        target->FillRect(7, 158, 306, 1, dark, false);
        if (focused)
        {
            target->FillRect(120, 161, 80, 13, dark, false);
            opts.foreground = light;
        }
        else
        {
            target->FillRect(120, 161, 80, 13, dark, false);
            target->FillRect(121, 162, 78, 11, light, false);
            opts.foreground = dark;
        }
        opts.horizontalAlign = PlatformAlign::Middle;
        opts.verticalAlign = PlatformAlign::Middle;
        target->DrawText(120, 161, 80, 13, "Back", &opts, false);
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

        Coord W = target->GetWidth(), H = target->GetHeight();
        PlatformDrawTextOptions opts{};
        opts.font = font;
        opts.verticalAlign = PlatformAlign::Middle;

        if (atMounts)
        {
            // --- Mount list view ---
            int raw = (int)list_mounts(mounts);
            nMounts = raw > 0 ? raw : 0;
            if (sel > nMounts)
                sel = nMounts;

            drawChrome(dc, target, "Files", "Mount Points", opts, W, H);

            if (nMounts == 0)
            {
                opts.horizontalAlign = PlatformAlign::Middle;
                opts.foreground = dark;
                target->DrawText(8, 38, 304, 11, "(no mounts)", &opts, false);
            }
            else
            {
                for (int row = 0; row < VIS && row < nMounts; row++)
                {
                    Coord ry = 38 + row * 12;
                    bool isSel = (sel == row);
                    if (isSel)
                    {
                        target->FillRect(8, ry, 304, 11, dark, false);
                        opts.foreground = light;
                    }
                    else
                    {
                        opts.foreground = dark;
                    }
                    // Mount path (null-terminate)
                    char pb[33];
                    int pl = mounts[row].path_len < 32 ? mounts[row].path_len : 32;
                    for (int j = 0; j < pl; j++)
                        pb[j] = (char)mounts[row].path[j];
                    pb[pl] = 0;
                    if (pl == 0)
                    {
                        pb[0] = '/';
                        pb[1] = 0;
                    }
                    opts.horizontalAlign = PlatformAlign::Begin;
                    target->DrawText(9, ry, 180, 11, (const mchar *)pb, &opts, false);
                    target->DrawText(210, ry, 90, 11, (const mchar *)fsType(mounts[row].fs_type), &opts, false);
                }
            }
            drawBack(target, sel == nMounts, opts);
        }
        else
        {
            // --- Directory listing view ---
            int raw = (int)list_dir_path((const unsigned char *)currentPath, entries);
            if (raw < 0)
                raw = 0;
            nEntries = 0;
            for (int i = 0; i < raw; i++)
            {
                unsigned char nl = entries[i].name_len;
                if (nl == 1 && entries[i].name[0] == '.')
                    continue;
                if (nl == 2 && entries[i].name[0] == '.' && entries[i].name[1] == '.')
                    continue;
                if (nEntries != i)
                    entries[nEntries] = entries[i];
                nEntries++;
            }
            int listItems = 1 + nEntries; // [..] + entries; Back at sel==listItems
            if (sel > listItems)
                sel = listItems;

            drawChrome(dc, target, "Files", (const mchar *)currentPath, opts, W, H);

            for (int row = 0; row < VIS; row++)
            {
                int vi = scrollTop + row;
                if (vi >= listItems)
                    break;
                Coord ry = 38 + row * 12;
                bool isSel = (sel == vi);
                if (isSel)
                {
                    target->FillRect(8, ry, 304, 11, dark, false);
                    opts.foreground = light;
                }
                else
                {
                    opts.foreground = dark;
                }
                opts.horizontalAlign = PlatformAlign::Begin;
                if (vi == 0)
                {
                    target->DrawText(9, ry, 290, 11, "[..]", &opts, false);
                }
                else
                {
                    int ei = vi - 1;
                    char nb[33];
                    int nl = entries[ei].name_len < 32 ? entries[ei].name_len : 32;
                    for (int j = 0; j < nl; j++)
                        nb[j] = (char)entries[ei].name[j];
                    nb[nl] = 0;
                    if (entries[ei].is_dir)
                    {
                        target->DrawText(9, ry, 8, 11, "/", &opts, false);
                        target->DrawText(17, ry, 190, 11, (const mchar *)nb, &opts, false);
                    }
                    else
                    {
                        target->DrawText(17, ry, 175, 11, (const mchar *)nb, &opts, false);
                        char sb[12];
                        u32str(entries[ei].size, sb);
                        target->DrawText(222, ry, 78, 11, (const mchar *)sb, &opts, false);
                    }
                }
            }
            drawBack(target, sel == listItems, opts);
        }
    }
};
