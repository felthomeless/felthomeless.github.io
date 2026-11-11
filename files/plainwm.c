#if 0
src=$(readlink -f "$0" 2>/dev/null) || src=$0
b=${TMPDIR:-/tmp}/wm-$(id -u)
if [ ! -x "$b" ] || [ "$src" -nt "$b" ]; then
    cc -x c -O2 -o "$b.new" "$src" -lX11 -lm -ldl || { echo "wm: needs gcc and libx11-dev" >&2; exit 1; }
    mv -f "$b.new" "$b"
fi
PLAINWM_SRC=$src
export PLAINWM_SRC
exec "$b" "$@"
#endif
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <dirent.h>
#include <ctype.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define KEYMOD Mod1Mask
#define WINMOD (Mod1Mask | Mod4Mask)
static const char *c_desktop = "";   /* desktop */
static const char *c_bar_focus = "#000000"; /* title bar, focused window */
static const char *c_bar_idle = "#000000";  /* title bar, unfocused windows */
static const char *c_text = "#f7bb23";      /* title bar text */
static const char *c_dock = "#000000";      /* dock tiles and cpu graph tile */
static const char *c_graph = "#f7bb23";     /* cpu graph bars */
static const char *c_panel = "#f7bb23";     /* run box, menu and low battery title */
static const char *c_ink = "#000000";       /* all other text */
static const char *font = "9x15bold", *wallpaper = "~/.wallpapers/d.jpg", *battery = "", *backlight = "", *sleep_state = "";
static const int bar_extra = 13, win_w = 360, win_h = 360, margin = 2, gap = 8, tile = 65, box = 100;
static const int stats_s = 3, low_bat = 40, bright_step = 1, idle_s = 90000;
static const unsigned char pmap[3] = { 3, 2, 1 };
static const unsigned btn_run = Button1, btn_menu = Button3, btn_move = Button1, btn_resize = Button3;

enum { NONE, SPAWN, RUN, MKDOCK, MENU, CLOSE, MAXI, FULL, MIN, UNMIN, NEXT, PREV, WS, BRIGHT, RESTART, QUIT, CLIP };
static const struct { unsigned m; const char *k; int a, n; const char *c; } keys[] = {
    { KEYMOD, "t", SPAWN, 0, "urxvt" }, { KEYMOD, "g", SPAWN, 0, "thunar" }, { KEYMOD, "r", SPAWN, 0, "urxvt -e sh wired" },
    { KEYMOD, "p", SPAWN, 0, "sudo poweroff" }, { KEYMOD, "m", BRIGHT, 1, NULL }, { KEYMOD, "n", BRIGHT, -1, NULL },
    { KEYMOD, "e", RUN, 0, NULL }, { KEYMOD, "w", MENU, 0, NULL }, { KEYMOD, "q", CLOSE, 0, NULL },
    { KEYMOD, "y", CLIP, 0, NULL }, { KEYMOD, "f", MAXI, 0, NULL }, { KEYMOD | ShiftMask, "f", FULL, 0, NULL }, { KEYMOD, "h", MIN, 0, NULL },
    { KEYMOD, "space", UNMIN, 0, NULL }, { KEYMOD, "a", NEXT, 0, NULL }, { KEYMOD, "d", PREV, 0, NULL },
    { KEYMOD, "x", WS, 0, NULL }, { KEYMOD, "c", WS, 1, NULL }, { KEYMOD, "v", WS, 2, NULL }, { KEYMOD, "b", WS, 3, NULL },
    { KEYMOD | ShiftMask, "r", RESTART, 0, NULL }, { KEYMOD | ShiftMask, "q", QUIT, 0, NULL } };
#define NKEYS ((int)(sizeof keys / sizeof keys[0]))
static const struct { const char *l; int a; const char *c; } menu[] = {
    { "browser", SPAWN, "firefox" }, { "torrent", SPAWN, "transmission-gtk" },
    { "audacious", SPAWN, "audacious" }, { "wpa_supplicant", SPAWN, "exec sudo wifi-up" },
    { "make dockapp", MKDOCK, NULL }, { "restart wm", RESTART, NULL }, { "exit wm", QUIT, NULL } };
#define NMENU ((int)(sizeof menu / sizeof menu[0]))
static const char *autostart[] = {
    "xrdb -load ~/.Xresources", "urxvt -e sh wired", "sudo tlp start", NULL };

#define MAXW 96
enum { DK_LAUNCH, DK_STATS, DK_CLOCK, DK_WATT, DK_VOL, DK_CLIP };
struct cl { Window w, t; int x, y, W, H, ws, on, pend, mode, min, sv[4]; long settle; };
struct dk { int kind, pic; Window w, main; char cmd[256]; };
static Display *dpy;
static Window root, dockw, rbw, mnw, alw, focus_w, dragw, statw, clockw, clipw, clipt, volt, wattw;
static Pixmap statpm;
static Atom a_nname, a_uri, a_cur, a_state, a_proto, a_del, a_own, a_st, a_nst, a_fs, a_clip, a_utf8, a_incr, a_paste, a_prim, psel;
static unsigned long pdesk, pfocus, pidle, ptext, pdock, pgraph, ppanel, pink;
static int scr, SW, SH, depth, cur, ncl, ndk, dcap, dw, rb_on, rbm, rbl, mn_on, mn_hi, mn_w, pst, incr, other;
static int fa, fd, th, bh, ih, dragging, dx0, dy0, dox, doy, dow, doh, al_on, al_dis, bat, bdis, cpu, ram;
static int hist[64], hn, asleep, lpx, lpy, vol = -1, vmute;
static double watts;
static unsigned long prevtot, previdle;
static long lastact, nexttick, saved_bl;
static char bat_dir[256], bl_dir[256], dock_path[512], rbt[8192], rbcmd[8192];
static char **argv_saved;
static struct cl cs[MAXW];
static struct dk *ds;
static XFontStruct *fnt;
static GC gc;
static Visual *vis;
static KeyCode kc[32];
#define MAXCLIP 1000
static char *clips[MAXCLIP], *cur_text;
static char clip_path[512];
static int clip_show;
static XFontStruct *sfnt;

static int nh, hidx, fx_base;
static Atom a_clipprop, a_targets;
static void die(const char *m) { fprintf(stderr, "wm: %s\n", m); exit(1); }
static int xerr(Display *d, XErrorEvent *e) {
    (void)d;
    if (e->error_code == BadAccess) other = 1;
    else if (e->error_code != BadWindow) fprintf(stderr, "wm: X error %d\n", e->error_code);
    return 0;
}
static Atom at(const char *n) { return XInternAtom(dpy, n, 0); }
static long now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1000L + t.tv_nsec / 1000000L; }
static unsigned long px(const char *n) { XColor c, e; return XAllocNamedColor(dpy, DefaultColormap(dpy, scr), n, &c, &e) ? c.pixel : BlackPixel(dpy, scr); }
static int tw(const char *s) { return XTextWidth(fnt, s, (int)strlen(s)); }
static void txt(Drawable d, int x, int y, const char *s, unsigned long f) { XSetForeground(dpy, gc, f); XDrawString(dpy, d, gc, x, y, s, (int)strlen(s)); }
static void spawn(const char *c) { if (c && *c && fork() == 0) { setsid(); execl("/bin/sh", "sh", "-c", c, (char *)0); _exit(127); } }
static int rd(const char *p, char *b, int n) { FILE *f = fopen(p, "r"); size_t k; if (!f) return 0; k = fread(b, 1, n - 1, f); fclose(f); b[k] = 0; return k > 0; }
static int first(const char *dir, const char *pre, char *o, size_t n) {
    DIR *d = opendir(dir); struct dirent *e; int ok = 0;
    if (!d) return 0;
    while (!ok && (e = readdir(d))) if (e->d_name[0] != '.' && !strncmp(e->d_name, pre, strlen(pre))) { snprintf(o, n, "%s/%s", dir, e->d_name); ok = 1; }
    closedir(d);
    return ok;
}
static int wi(Window w) { int i; for (i = 0; i < ncl; i++) if (cs[i].w == w) return i; return -1; }
static int ti(Window w) { int i; for (i = 0; i < ncl; i++) if (cs[i].t == w) return i; return -1; }
static int di(Window w) { int i; for (i = 0; i < ndk; i++) if (ds[i].w == w) return i; return -1; }
static int own(Window w) { return w == dockw || w == rbw || w == mnw || w == alw; }
static int top_on(void) { int i; for (i = ncl - 1; i >= 0; i--) if (cs[i].ws == cur && !cs[i].min) return i; return -1; }
static const char *name_of(Window w) { static char b[256]; char *s = 0; b[0] = 0; if (XFetchName(dpy, w, &s) && s) { snprintf(b, sizeof b, "%s", s); XFree(s); } return b; }
static void set_st(Window w, long st) { long d[2] = { st, None }; XChangeProperty(dpy, w, a_state, a_state, 32, PropModeReplace, (unsigned char *)d, 2); }
static void own_mark(Window w) { long one = 1; XChangeProperty(dpy, w, a_own, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&one, 1); }
static void dock_save(const char *img, const char *cmd) { FILE *f = fopen(dock_path, "a"); if (f) { fprintf(f, "%s\t%s\n", img, cmd); fclose(f); } }
static char *trim(char *s) { char *e; while (*s == ' ' || *s == '\t') s++; e = s + strlen(s); while (e > s && (e[-1] == ' ' || e[-1] == '\t')) *--e = 0; return s; }

static unsigned char cl8(double v) { int i = (int)floor(v + 0.5); return (unsigned char)(i < 0 ? 0 : i > 255 ? 255 : i); }
static const unsigned char zz[64] = { 0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5, 12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63 };
struct hf { int mx[17], mn[17], vp[17]; unsigned char v[256]; };
struct jc { int id, h, v, tq, td, ta, pr, bw, bh; short *co; };
struct br { const unsigned char *p; long n, i; unsigned b; int c, m; };
static unsigned short jq[4][64];
static struct hf jh[2][4];
static float ct8[8][8];
static int eobrun;
static int bit(struct br *r) {
    if (!r->c) {
        int v = 0;
        if (!r->m && r->i < r->n) {
            v = r->p[r->i++];
            if (v == 0xff) { if (r->i < r->n && r->p[r->i] == 0) r->i++; else { r->m = 1; r->i--; v = 0; } }
        }
        r->b = (unsigned)v; r->c = 8;
    }
    return (int)((r->b >> --r->c) & 1);
}
static int bits(struct br *r, int n) { int v = 0; while (n-- > 0) v = v << 1 | bit(r); return v; }
static int ext(int v, int n) { return n && v < (1 << (n - 1)) ? v - (1 << n) + 1 : v; }
static int hdec(struct br *r, const struct hf *h) {
    int code = 0, l;
    for (l = 1; l <= 16; l++) { code = code << 1 | bit(r); if (h->mx[l] >= 0 && code <= h->mx[l]) return h->v[h->vp[l] + code - h->mn[l]]; }
    return 0;
}
static int dht(const unsigned char *s, long len) {
    long k = 0;
    while (k + 17 <= len) {
        int tc = s[k] >> 4, id = s[k] & 15, l, code = 0, n = 0, c[17];
        struct hf *h;
        if (tc > 1 || id > 3) return 0;
        h = &jh[tc][id];
        for (l = 1; l <= 16; l++) { c[l] = s[k + l]; n += c[l]; }
        if (n > 256 || k + 17 + n > len) return 0;
        memcpy(h->v, s + k + 17, (size_t)n);
        k += 17 + n;
        for (n = 0, l = 1; l <= 16; l++) {
            if (c[l]) { h->vp[l] = (int)n; h->mn[l] = code; code += c[l]; n += c[l]; h->mx[l] = code - 1; }
            else h->mx[l] = -1;
            code <<= 1;
        }
    }
    return 1;
}
static int dqt(const unsigned char *s, long len) {
    long k = 0;
    while (k < len) {
        int pq = s[k] >> 4, t = s[k] & 3, i, sz = pq ? 128 : 64;
        k++;
        if (k + sz > len) return 0;
        for (i = 0; i < 64; i++) jq[t][zz[i]] = pq ? (unsigned short)(s[k + 2 * i] << 8 | s[k + 2 * i + 1]) : s[k + i];
        k += sz;
    }
    return 1;
}
static void idct8(const float *in, unsigned char *out, int stride) {
    float t[64]; int x, y, u, v;
    for (v = 0; v < 8; v++) for (x = 0; x < 8; x++) { float s = 0; for (u = 0; u < 8; u++) s += ct8[x][u] * in[v * 8 + u]; t[v * 8 + x] = s; }
    for (y = 0; y < 8; y++) for (x = 0; x < 8; x++) {
        float s = 0; int p; for (v = 0; v < 8; v++) s += ct8[y][v] * t[v * 8 + x];
        p = (int)floorf(s + 128.5f);
        out[y * stride + x] = (unsigned char)(p < 0 ? 0 : p > 255 ? 255 : p);
    }
}
static void rst(struct br *r) {
    r->c = 0; r->m = 0;
    while (r->i + 1 < r->n && !(r->p[r->i] == 0xff && r->p[r->i + 1] >= 0xd0 && r->p[r->i + 1] <= 0xd7)) r->i++;
    r->i += 2;
}
/* one block of one scan: DC first/refine, AC first/refine (progressive and baseline) */
static int dec_block(struct br *r, struct jc *c, int bx, int by, int ss, int se, int ah, int al, int hdc, int hac) {
    short *co = c->co + ((size_t)by * c->bw + bx) * 64;
    int k, t, rs, s, rr, b1 = 1 << al;
    if (ss == 0) {
        if (ah == 0) { t = hdec(r, &jh[0][hdc]); if (t > 11) return 0; c->pr += ext(bits(r, t), t); co[0] = (short)(c->pr << al); }
        else if (bit(r)) co[0] = (short)(co[0] | b1);
        if (se == 0) return 1;
        ss = 1;
    }
    if (ah == 0) {
        if (eobrun > 0) { eobrun--; return 1; }
        for (k = ss; k <= se; ) {
            rs = hdec(r, &jh[1][hac]); s = rs & 15; rr = rs >> 4;
            if (!s) {
                if (rr < 15) { eobrun = (1 << rr) - 1; if (rr) eobrun += bits(r, rr); break; }
                k += 16;
            } else {
                k += rr;
                if (k > 63) return 0;
                co[zz[k]] = (short)(ext(bits(r, s), s) * (1 << al));
                k++;
            }
        }
        return 1;
    }
    if (eobrun > 0) {
        eobrun--;
        for (k = ss; k <= se; k++) { short *p = &co[zz[k]]; if (*p && bit(r) && !(*p & b1)) *p = (short)(*p > 0 ? *p + b1 : *p - b1); }
        return 1;
    }
    k = ss;
    do {
        int v;
        rs = hdec(r, &jh[1][hac]); s = rs & 15; rr = rs >> 4;
        if (!s) { if (rr < 15) { eobrun = (1 << rr) - 1; if (rr) eobrun += bits(r, rr); rr = 64; } }
        else v = bit(r) ? b1 : -b1;
        while (k <= se) {
            short *p = &co[zz[k++]];
            if (*p) { if (bit(r) && !(*p & b1)) *p = (short)(*p > 0 ? *p + b1 : *p - b1); }
            else { if (rr == 0) { if (s) *p = (short)v; break; } rr--; }
        }
    } while (k <= se);
    return 1;
}
static int scan(struct br *r, struct jc *c, const int *sc, int ns, int ss, int se, int ah, int al, int ri, int mx, int my, int w, int h, int hm, int vm) {
    int u, units, i, j;
    eobrun = 0;
    if (ns == 1) {
        struct jc *k = &c[sc[0]];
        int bw1 = ((w * k->h + hm - 1) / hm + 7) / 8;
        units = bw1 * (((h * k->v + vm - 1) / vm + 7) / 8);
        for (u = 0; u < units; u++) {
            if (ri && u && u % ri == 0) { rst(r); eobrun = 0; for (i = 0; i < ns; i++) c[sc[i]].pr = 0; }
            if (!dec_block(r, k, u % bw1, u / bw1, ss, se, ah, al, k->td, k->ta)) return 0;
        }
    } else {
        units = mx * my;
        for (u = 0; u < units; u++) {
            if (ri && u && u % ri == 0) { rst(r); eobrun = 0; for (i = 0; i < ns; i++) c[sc[i]].pr = 0; }
            for (i = 0; i < ns; i++) {
                struct jc *k = &c[sc[i]];
                for (j = 0; j < k->h * k->v; j++)
                    if (!dec_block(r, k, (u % mx) * k->h + j % k->h, (u / mx) * k->v + j / k->h, ss, se, ah, al, k->td, k->ta)) return 0;
            }
        }
    }
    return 1;
}
static unsigned char *to_rgb(unsigned char **pl, const int *pw, const struct jc *c, int nc, int hm, int vm, int w, int h) {
    unsigned char *o = malloc((size_t)w * h * 3); int x, y, i;
    if (!o) return 0;
    for (y = 0; y < h; y++) for (x = 0; x < w; x++) {
        int v[3] = { 0, 0, 0 }; unsigned char *q = o + ((size_t)y * w + x) * 3;
        for (i = 0; i < nc; i++) v[i] = pl[i][(size_t)(y * c[i].v / vm) * pw[i] + x * c[i].h / hm];
        if (nc == 1) q[0] = q[1] = q[2] = (unsigned char)v[0];
        else { double Y = v[0], cb = v[1] - 128.0, cr = v[2] - 128.0; q[0] = cl8(Y + 1.402 * cr); q[1] = cl8(Y - 0.344136 * cb - 0.714136 * cr); q[2] = cl8(Y + 1.772 * cb); }
    }
    return o;
}
static unsigned char *jpg(const unsigned char *d, long n, int *W, int *H) {
    struct jc c[3]; int nc = 0, hm = 1, vm = 1, mx = 0, my = 0, ri = 0, w = 0, h = 0, ok = 0, i;
    long p = 2; unsigned char *out = 0, *pl[3] = { 0, 0, 0 }; int pw[3] = { 0, 0, 0 };
    static int inited;
    if (!inited) { int x, u; for (x = 0; x < 8; x++) for (u = 0; u < 8; u++) ct8[x][u] = (float)((u ? 1.0 : 1.0 / sqrt(2.0)) * cos((2 * x + 1) * u * 3.14159265358979323846 / 16.0) / 2.0); inited = 1; }
    memset(c, 0, sizeof c); memset(jq, 0, sizeof jq); memset(jh, 0xff, sizeof jh);
    while (p + 4 <= n) {
        int m, len; const unsigned char *s;
        if (d[p] != 0xff) { p++; continue; }
        m = d[p + 1];
        if (m == 0xff) { p++; continue; }
        p += 2;
        if (m == 0xd9) break;
        if (m == 0 || m == 1 || (m >= 0xd0 && m <= 0xd7)) continue;
        len = d[p] << 8 | d[p + 1];
        if (len < 2 || p + len > n) break;
        s = d + p + 2;
        switch (m) {
        case 0xdb: if (!dqt(s, len - 2)) goto done; break;
        case 0xc4: if (!dht(s, len - 2)) goto done; break;
        case 0xdd: ri = s[0] << 8 | s[1]; break;
        case 0xc0: case 0xc1: case 0xc2:
            if (nc || s[0] != 8) goto done;
            h = s[1] << 8 | s[2]; w = s[3] << 8 | s[4]; nc = s[5];
            if ((nc != 1 && nc != 3) || !w || !h) goto done;
            for (i = 0; i < nc; i++) {
                c[i].id = s[6 + 3 * i]; c[i].h = s[7 + 3 * i] >> 4; c[i].v = s[7 + 3 * i] & 15; c[i].tq = s[8 + 3 * i] & 3;
                if (c[i].h < 1 || c[i].h > 4 || c[i].v < 1 || c[i].v > 4) goto done;
                if (c[i].h > hm) hm = c[i].h;
                if (c[i].v > vm) vm = c[i].v;
            }
            mx = (w + 8 * hm - 1) / (8 * hm); my = (h + 8 * vm - 1) / (8 * vm);
            for (i = 0; i < nc; i++) {
                c[i].bw = mx * c[i].h; c[i].bh = my * c[i].v;
                c[i].co = calloc((size_t)c[i].bw * c[i].bh * 64, sizeof(short));
                if (!c[i].co) goto done;
            }
            break;
        case 0xda: {
            struct br r; int sc[3], ns = s[0], q, id, ss, se, ah, al;
            if (!nc || ns < 1 || ns > 3 || len < 6 + 2 * ns) goto done;
            for (i = 0; i < ns; i++) {
                id = s[1 + 2 * i];
                for (q = 0; q < nc && c[q].id != id; q++) ;
                if (q == nc) goto done;
                sc[i] = q; c[q].td = s[2 + 2 * i] >> 4; c[q].ta = s[2 + 2 * i] & 15;
                if (c[q].td > 3 || c[q].ta > 3) goto done;
                c[q].pr = 0;
            }
            ss = s[1 + 2 * ns]; se = s[2 + 2 * ns]; ah = s[3 + 2 * ns] >> 4; al = s[3 + 2 * ns] & 15;
            if (ss > 63 || se > 63 || ss > se) goto done;
            r.p = d; r.n = n; r.i = p + len; r.b = 0; r.c = 0; r.m = 0;
            if (!scan(&r, c, sc, ns, ss, se, ah, al, ri, mx, my, w, h, hm, vm)) goto done;
            ok = 1;
            p = r.i;
            continue;
        }
        default: break;
        }
        p += len;
    }
done:
    if (ok) {
        float f[64]; int by, bx, k;
        for (i = 0; i < nc && ok; i++) {
            pw[i] = c[i].bw * 8;
            pl[i] = malloc((size_t)pw[i] * c[i].bh * 8);
            if (!pl[i]) { ok = 0; break; }
            for (by = 0; by < c[i].bh; by++) for (bx = 0; bx < c[i].bw; bx++) {
                short *co = c[i].co + ((size_t)by * c[i].bw + bx) * 64;
                for (k = 0; k < 64; k++) f[k] = (float)(co[k] * jq[c[i].tq][k]);
                idct8(f, pl[i] + (size_t)by * 8 * pw[i] + bx * 8, pw[i]);
            }
        }
        if (ok) { out = to_rgb(pl, pw, c, nc, hm, vm, w, h); if (out) { *W = w; *H = h; } }
    }
    for (i = 0; i < nc; i++) { free(c[i].co); free(pl[i]); }
    return out;
}
struct bs { const unsigned char *p; long n, i; int b, c; };
static int bitin(struct bs *s, int k) {
    int v = 0, j;
    for (j = 0; j < k; j++) {
        if (!s->c) { if (s->i >= s->n) return -1; s->b = s->p[s->i++]; s->c = 8; }
        v |= (s->b & 1) << j; s->b >>= 1; s->c--;
    }
    return v;
}
struct pf { short cnt[16], sym[288]; };
static int pbuild(struct pf *h, const short *len, int n) {
    int s, l, left = 1, off[16];
    for (l = 0; l < 16; l++) h->cnt[l] = 0;
    for (s = 0; s < n; s++) h->cnt[len[s]]++;
    for (l = 1; l < 16; l++) { left = left * 2 - h->cnt[l]; if (left < 0) return -1; }
    off[1] = 0;
    for (l = 1; l < 15; l++) off[l + 1] = off[l] + h->cnt[l];
    for (s = 0; s < n; s++) if (len[s]) h->sym[off[len[s]]++] = (short)s;
    return 0;
}
static int pdec(struct bs *s, const struct pf *h) {
    int l, code = 0, first = 0, idx = 0, c;
    for (l = 1; l < 16; l++) {
        if ((c = bitin(s, 1)) < 0) return -1;
        code |= c; c = h->cnt[l];
        if (code - c < first) return h->sym[idx + code - first];
        idx += c; first = (first + c) << 1; code <<= 1;
    }
    return -1;
}
static const short LB[29] = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
static const short LE[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0 };
static const short DB[30] = { 1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577 };
static const short DE[30] = { 0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };
static const short CO[19] = { 16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15 };
static int pcodes(struct bs *s, unsigned char *o, long *op, long cap, const struct pf *lc, const struct pf *dc) {
    int sym, len, e; long dist;
    for (;;) {
        if ((sym = pdec(s, lc)) < 0) return -1;
        if (sym < 256) { if (*op >= cap) return -1; o[(*op)++] = (unsigned char)sym; }
        else if (sym == 256) return 0;
        else {
            if (sym > 285) return -1;
            sym -= 257;
            if ((e = bitin(s, LE[sym])) < 0) return -1;
            len = LB[sym] + e;
            if ((sym = pdec(s, dc)) < 0 || sym > 29) return -1;
            if ((e = bitin(s, DE[sym])) < 0) return -1;
            dist = DB[sym] + e;
            if (dist > *op || *op + len > cap) return -1;
            while (len--) { o[*op] = o[*op - dist]; (*op)++; }
        }
    }
}
static long pinf(const unsigned char *src, long n, unsigned char *o, long cap) {
    struct bs s = { src, n, 0, 0 };
    struct pf lc, dc, cd;
    short len[320];
    long op = 0;
    int last, type, i, v, sym, nl, nd, nc, rep;
    do {
        if ((last = bitin(&s, 1)) < 0 || (type = bitin(&s, 2)) < 0) return -1;
        if (type == 0) {
            s.c = 0;
            if (s.i + 4 > n) return -1;
            v = src[s.i] | src[s.i + 1] << 8;
            s.i += 4;
            if (s.i + v > n || op + v > cap) return -1;
            memcpy(o + op, src + s.i, v);
            op += v; s.i += v;
        } else if (type == 1) {
            for (i = 0; i < 288; i++) len[i] = i < 144 ? 8 : i < 256 ? 9 : i < 280 ? 7 : 8;
            pbuild(&lc, len, 288);
            for (i = 0; i < 30; i++) len[i] = 5;
            pbuild(&dc, len, 30);
            if (pcodes(&s, o, &op, cap, &lc, &dc)) return -1;
        } else if (type == 2) {
            if ((nl = bitin(&s, 5)) < 0 || (nd = bitin(&s, 5)) < 0 || (nc = bitin(&s, 4)) < 0) return -1;
            nl += 257; nd += 1; nc += 4;
            for (i = 0; i < 19; i++) len[i] = 0;
            for (i = 0; i < nc; i++) { if ((v = bitin(&s, 3)) < 0) return -1; len[CO[i]] = (short)v; }
            if (pbuild(&cd, len, 19)) return -1;
            for (i = 0; i < nl + nd;) {
                if ((sym = pdec(&s, &cd)) < 0) return -1;
                if (sym < 16) { len[i++] = (short)sym; continue; }
                if (sym == 16) { if (!i) return -1; v = len[i - 1]; rep = 3 + bitin(&s, 2); }
                else if (sym == 17) { v = 0; rep = 3 + bitin(&s, 3); }
                else { v = 0; rep = 11 + bitin(&s, 7); }
                if (rep < 0 || i + rep > nl + nd) return -1;
                while (rep--) len[i++] = (short)v;
            }
            if (pbuild(&lc, len, nl) || pbuild(&dc, len + nl, nd)) return -1;
            if (pcodes(&s, o, &op, cap, &lc, &dc)) return -1;
        } else return -1;
    } while (!last);
    return op;
}
static long be32(const unsigned char *s) { return (long)s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3]; }
static int paeth(int a, int b, int c) {
    int p = a + b - c, pa = abs(p - a), pb = abs(p - b), pc = abs(p - c);
    return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
}
static unsigned char *png(const unsigned char *d, long n, int *W, int *H) {
    unsigned char *z = malloc((size_t)n), *raw = 0, *o = 0;
    const unsigned char *pal = 0;
    long p = 8, zl = 0, pn = 0, len, cap;
    int w = 0, h = 0, ct = 0, ch, stride, x, y, k, ft;
    if (!z) return 0;
    while (p + 12 <= n) {
        const unsigned char *t = d + p + 4, *s = d + p + 8;
        len = be32(d + p);
        if (len < 0 || p + 12 + len > n) break;
        if (!memcmp(t, "IHDR", 4)) { w = (int)be32(s); h = (int)be32(s + 4); ct = s[9]; if (s[8] != 8 || s[12]) goto out; }
        else if (!memcmp(t, "PLTE", 4)) { pal = s; pn = len; }
        else if (!memcmp(t, "IDAT", 4)) { memcpy(z + zl, s, (size_t)len); zl += len; }
        else if (!memcmp(t, "IEND", 4)) break;
        p += 12 + len;
    }
    ch = ct == 0 ? 1 : ct == 2 ? 3 : ct == 3 ? 1 : ct == 4 ? 2 : ct == 6 ? 4 : 0;
    if (!ch || w < 1 || h < 1 || zl < 2 || (ct == 3 && !pal)) goto out;
    stride = w * ch;
    cap = (long)h * (stride + 1);
    raw = malloc((size_t)cap);
    if (!raw || pinf(z + 2, zl - 2, raw, cap) != cap) goto out;
    for (y = 0; y < h; y++) {
        unsigned char *r = raw + (long)y * (stride + 1) + 1, *u = y ? r - (stride + 1) : 0;
        ft = raw[(long)y * (stride + 1)];
        for (x = 0; x < stride; x++) {
            int a = x >= ch ? r[x - ch] : 0, b = y ? u[x] : 0, c = y && x >= ch ? u[x - ch] : 0;
            switch (ft) {
            case 1: r[x] += a; break;
            case 2: r[x] += b; break;
            case 3: r[x] += (a + b) / 2; break;
            case 4: r[x] += paeth(a, b, c); break;
            }
        }
    }
    o = malloc((size_t)w * h * 3);
    if (!o) goto out;
    for (y = 0; y < h; y++) for (x = 0; x < w; x++) {
        const unsigned char *q = raw + (long)y * (stride + 1) + 1 + (long)x * ch;
        unsigned char *c3 = o + ((long)y * w + x) * 3;
        int r = q[0], g = q[0], b = q[0], a = 255;
        if (ct == 2 || ct == 6) { g = q[1]; b = q[2]; }
        if (ct == 4) a = q[1];
        if (ct == 6) a = q[3];
        if (ct == 3) { k = q[0] * 3; r = g = b = 0; if (k + 2 < pn) { r = pal[k]; g = pal[k + 1]; b = pal[k + 2]; } }
        c3[0] = (unsigned char)(r * a / 255); c3[1] = (unsigned char)(g * a / 255); c3[2] = (unsigned char)(b * a / 255);
    }
    *W = w; *H = h;
out:
    free(z); free(raw);
    return o;
}
static unsigned char *img_rgb(const char *path, int *W, int *H) {
    char p[512]; FILE *f; long n; unsigned char *b = 0, *o = 0;
    if (path[0] == '~' && path[1] == '/') snprintf(p, sizeof p, "%s/%s", getenv("HOME") ? getenv("HOME") : ".", path + 2);
    else snprintf(p, sizeof p, "%s", path);
    f = fopen(p, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) == 0 && (n = ftell(f)) > 8) {
        rewind(f);
        b = malloc((size_t)n);
        if (b && fread(b, 1, (size_t)n, f) == (size_t)n) {
            if (b[0] == 0xff && b[1] == 0xd8) o = jpg(b, n, W, H);
            else if (b[0] == 137 && b[1] == 'P') o = png(b, n, W, H);
        }
        free(b);
    }
    fclose(f);
    return o;
}
static Pixmap pic(const char *path, int W, int H) {
    int sw = 0, sh = 0, x, y; unsigned char *rgb; Pixmap pm;
    if ((depth != 24 && depth != 32) || ImageByteOrder(dpy) != LSBFirst) return 0;
    rgb = img_rgb(path, &sw, &sh);
    if (!rgb) return 0;
    pm = XCreatePixmap(dpy, root, W, H, depth);
    for (y = 0; y < H; y += 16) {
        int r, n = H - y < 16 ? H - y : 16; unsigned char *o = malloc((size_t)W * n * 4); XImage *im;
        if (!o) break;
        for (r = 0; r < n; r++) {
            const unsigned char *s = rgb + (size_t)((long)(y + r) * sh / H) * sw * 3;
            for (x = 0; x < W; x++) {
                const unsigned char *q = s + (size_t)((long)x * sw / W) * 3; unsigned char *d = o + ((size_t)r * W + x) * 4;
                d[0] = q[2]; d[1] = q[1]; d[2] = q[0]; d[3] = 0;
            }
        }
        im = XCreateImage(dpy, vis, depth, ZPixmap, 0, (char *)o, W, n, 32, 0);
        XPutImage(dpy, pm, gc, im, 0, 0, 0, y, W, n);
        XDestroyImage(im);
    }
    free(rgb);
    return pm;
}
static double sysval(const char *dir, const char *f) { char p[320], b[32]; snprintf(p, sizeof p, "%s/%s", dir, f); return rd(p, b, sizeof b) ? atof(b) : 0; }
static void bat_read(void) {
    char p[320], b[32]; bat = -1; bdis = 0; watts = 0;
    if (!bat_dir[0]) return;
    snprintf(p, sizeof p, "%s/capacity", bat_dir); if (rd(p, b, sizeof b)) bat = atoi(b);
    snprintf(p, sizeof p, "%s/status", bat_dir); bdis = rd(p, b, sizeof b) && !strncmp(b, "Discharging", 11);
    watts = sysval(bat_dir, "power_now") / 1e6;
    if (watts <= 0) watts = sysval(bat_dir, "current_now") * sysval(bat_dir, "voltage_now") / 1e12;
    if (watts < 0) watts = -watts;
}
static long bl_get(void) { char p[320], b[32]; if (!bl_dir[0]) return -1; snprintf(p, sizeof p, "%s/brightness", bl_dir); return rd(p, b, sizeof b) ? atol(b) : -1; }
static void bl_put(long v) { char p[320]; FILE *f; if (!bl_dir[0]) return; snprintf(p, sizeof p, "%s/brightness", bl_dir); if ((f = fopen(p, "w"))) { fprintf(f, "%ld\n", v); fclose(f); } }
static void bright(int d) {
    char p[320], b[32]; long mx, v, st;
    if (!bl_dir[0]) return;
    snprintf(p, sizeof p, "%s/max_brightness", bl_dir);
    if (!rd(p, b, sizeof b) || (mx = atol(b)) <= 0 || (v = bl_get()) < 0) return;
    st = mx * bright_step / 100; if (st < 1) st = 1;
    v += d * st; bl_put(v < 0 ? 0 : v > mx ? mx : v);
}
static void alert_show(int on) { al_on = on; if (on) { XMoveResizeWindow(dpy, alw, (SW - box) / 2, margin, box, 40); XMapRaised(dpy, alw); } else XUnmapWindow(dpy, alw); }
static void alert_draw(void) { char s[40]; snprintf(s, sizeof s, "alert low batt %d%%", bat); XClearWindow(dpy, alw); txt(alw, 10, 26, s, pink); }
static void stats_read(void) {
    char b[256], k[64]; unsigned long v[8] = { 0 }, t = 0, id, mt = 0, mf = 0, mb = 0, mc = 0, ms = 0, q; FILE *f = fopen("/proc/stat", "r"); int i;
    if (f) {
        if (fgets(b, sizeof b, f) && sscanf(b, "cpu %lu %lu %lu %lu %lu %lu %lu %lu", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7]) >= 4) {
            for (i = 0; i < 8; i++) t += v[i];
            id = v[3] + v[4];
            if (t > prevtot) cpu = (int)(100 * ((t - prevtot) - (id - previdle)) / (t - prevtot));
            prevtot = t; previdle = id;
            hist[hn % 64] = cpu; hn++;
        }
        fclose(f);
    }
    f = fopen("/proc/meminfo", "r");
    if (f) {
        while (fgets(b, sizeof b, f)) if (sscanf(b, "%63s %lu", k, &q) == 2) {
            if (!strcmp(k, "MemTotal:")) mt = q; else if (!strcmp(k, "MemFree:")) mf = q;
            else if (!strcmp(k, "Buffers:")) mb = q; else if (!strcmp(k, "Cached:")) mc = q;
            else if (!strcmp(k, "SReclaimable:")) ms = q;
        }
        fclose(f);
        ram = (int)((mt > mf + mb + mc + ms ? mt - mf - mb - mc - ms : 0) / 1024);
    }
}
static void stats_draw(Window w) {
    char s[32]; int i, n = hn < 56 ? hn : 56, y0 = 40, H = tile - y0 - 3; XRectangle r[56];
    Drawable d = statpm ? statpm : w;
    XSetForeground(dpy, gc, pdock); XFillRectangle(dpy, d, gc, 0, 0, tile, tile);
    snprintf(s, sizeof s, "cpu %d%%", cpu); txt(d, 3, 11, s, ptext);
    snprintf(s, sizeof s, "ram %dM", ram); txt(d, 3, 22, s, ptext);
    if (bat >= 0) snprintf(s, sizeof s, "bat %d%%", bat); else snprintf(s, sizeof s, "bat --");
    txt(d, 3, 33, s, ptext);
    XSetForeground(dpy, gc, ptext); XDrawRectangle(dpy, d, gc, 2, y0 - 1, tile - 5, H + 2);
    XSetForeground(dpy, gc, pgraph);
    for (i = 0; i < n; i++) { int h = hist[(hn - n + i) % 64] * H / 100; r[i].x = 3 + i; r[i].y = y0 + H - h; r[i].width = 1; r[i].height = h; }
    XFillRectangles(dpy, d, gc, r, n);
    if (statpm) XCopyArea(dpy, statpm, w, gc, 0, 0, tile, tile, 0, 0);
}
static void wake(void) {
    asleep = 0; XUngrabKeyboard(dpy, CurrentTime); XUngrabPointer(dpy, CurrentTime);
    if (saved_bl >= 0) bl_put(saved_bl);
    lastact = now();
}
static void sleep_now(void) {
    FILE *f; asleep = 1; saved_bl = bl_get(); bl_put(0);
    XGrabKeyboard(dpy, root, False, GrabModeAsync, GrabModeAsync, CurrentTime);
    XGrabPointer(dpy, root, False, ButtonPressMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    XFlush(dpy);
    if (*sleep_state && (f = fopen("/sys/power/state", "w"))) { fputs(sleep_state, f); fclose(f); }
    wake();
}
static void idle(void) {
    Window r, c; int rx, ry, wx, wy, i, act = 0; unsigned m; char k[32];
    if (bat < 0 || !bdis) { lastact = now(); return; }
    XQueryPointer(dpy, root, &r, &c, &rx, &ry, &wx, &wy, &m);
    XQueryKeymap(dpy, k);
    for (i = 0; i < 32; i++) if (k[i]) act = 1;
    if (act || rx != lpx || ry != lpy || (m & (Button1Mask | Button2Mask | Button3Mask))) lastact = now();
    lpx = rx; lpy = ry;
    if (now() - lastact >= idle_s * 1000L) sleep_now();
}
static char lastc[16], lastw[16];
static void clock_draw(Window w) {
    char s[16]; time_t t = time(0); struct tm *tm = localtime(&t);
    strftime(s, sizeof s, "%H %M", tm);
    if (!strcmp(s, lastc)) return;
    strcpy(lastc, s);
    XSetWindowBackground(dpy, w, pdock); XClearWindow(dpy, w);
    txt(w, (tile - tw(s)) / 2, 26, s, ptext);
    strftime(s, sizeof s, "%a %d", tm); txt(w, (tile - tw(s)) / 2, 46, s, ptext);
}
static void watt_draw(Window w) {
    char s[16];
    if (bat < 0) snprintf(s, sizeof s, "-- W"); else snprintf(s, sizeof s, "%.1f W", watts);
    if (!strcmp(s, lastw)) return;
    strcpy(lastw, s);
    XSetWindowBackground(dpy, w, pdock); XClearWindow(dpy, w);
    txt(w, (tile - tw(s)) / 2, 36, s, ptext);
}
static void vol_draw(Window w);
static int pulse;
static char dsink[256];
static const char *vol_get(void) {
    return pulse ? "pactl info 2>/dev/null; pactl list sinks 2>/dev/null" : "amixer get Master 2>/dev/null";
}
static void vol_parse(FILE *f) {
    char b[512], *t, *p; int in = 0;
    while (fgets(b, sizeof b, f)) {
        t = b + strspn(b, " \t");
        if (!pulse) {
            if ((p = strchr(b, '%')) && vol < 0) { char *q = p; while (q > b && isdigit((unsigned char)q[-1])) q--; vol = atoi(q); }
            if (strstr(b, "[off]")) vmute = 1;
            continue;
        }
        if (!strncmp(t, "Default Sink: ", 14)) { t[strcspn(t, "\n")] = 0; snprintf(dsink, sizeof dsink, "%s", t + 14); continue; }
        if (!strncmp(t, "Name: ", 6)) { t[strcspn(t, "\n")] = 0; in = !strcmp(t + 6, dsink); continue; }
        if (!in) continue;
        if (!strncmp(t, "Mute: yes", 9)) vmute = 1;
        if (!strncmp(t, "Volume: ", 8) && vol < 0 && (p = strchr(t, '%'))) { char *q = p; while (q > t && isdigit((unsigned char)q[-1])) q--; vol = atoi(q); }
    }
}
static void vol_run(const char *set) {
    char c[400]; FILE *f;
    snprintf(c, sizeof c, "%s; %s", set, vol_get());
    vol = -1; vmute = 0;
    f = popen(c, "r");
    if (f) { vol_parse(f); pclose(f); }
    if (volt) vol_draw(volt);
}
static void vol_read(void) {
    FILE *f; vol = -1; vmute = 0;
    f = popen(vol_get(), "r");
    if (f) { vol_parse(f); pclose(f); }
}
static void vol_draw(Window w) {
    char s[16]; int fill = vol > 0 ? (vol > 100 ? 100 : vol) * 38 / 100 : 0;
    XSetWindowBackground(dpy, w, pdock); XClearWindow(dpy, w);
    if (vmute) snprintf(s, sizeof s, "mute"); else if (vol < 0) snprintf(s, sizeof s, "--"); else snprintf(s, sizeof s, "%d", vol);
    txt(w, (tile - tw(s)) / 2, 12, s, ptext);
    XSetForeground(dpy, gc, pink); XFillRectangle(dpy, w, gc, 22, 18, 20, 40);
    XSetForeground(dpy, gc, ptext); XDrawRectangle(dpy, w, gc, 22, 18, 20, 40);
    if (!vmute && fill > 0) XFillRectangle(dpy, w, gc, 23, 57 - fill, 18, fill);
}
static void vol_set(int delta) {
    char c[160];
    if (pulse) snprintf(c, sizeof c, "pactl set-sink-mute @DEFAULT_SINK@ 0 2>/dev/null; pactl set-sink-volume @DEFAULT_SINK@ %+d%% 2>/dev/null", delta);
    else snprintf(c, sizeof c, "amixer -q sset Master %d%%%c unmute", delta < 0 ? -delta : delta, delta < 0 ? '-' : '+');
    vol_run(c);
}
static void vol_toggle(void) { vol_run(pulse ? "pactl set-sink-mute @DEFAULT_SINK@ toggle 2>/dev/null" : "amixer -q sset Master toggle"); }
static void vol_sync(void) {
    vol_read();
    if (volt) vol_draw(volt);
}
static void clip_draw(Window w) {
    char s[16], p[12]; int i, k = nh ? hidx : 0;
    XSetWindowBackground(dpy, w, pdock); XClearWindow(dpy, w);
    XSetFont(dpy, gc, sfnt->fid);
    if (!clip_show || !nh) txt(w, 22, 36, "clip", ptext);
    else {
        snprintf(s, sizeof s, "%d/%d", k + 1, nh); txt(w, 3, 10, s, ptext);
        for (i = 0; i < 5 && (size_t)(i * 11) < strlen(clips[k]); i++) {
            int j; for (j = 0; j < 11 && clips[k][i * 11 + j]; j++) p[j] = clips[k][i * 11 + j] < 32 ? ' ' : clips[k][i * 11 + j];
            p[j] = 0;
            txt(w, 3, 20 + i * 9, p, ptext);
        }
    }
    XSetFont(dpy, gc, fnt->fid);
}
static void tick(void) {
    stats_read(); bat_read(); vol_sync();
    if (statw) stats_draw(statw);
    if (clockw) clock_draw(clockw);
    if (wattw) watt_draw(wattw);
    if (bat >= 0 && bat <= low_bat && bdis) { if (!al_on && !al_dis) alert_show(1); else if (al_on) alert_draw(); }
    else { al_dis = 0; if (al_on) alert_show(0); }
    idle();
    nexttick = now() + stats_s * 1000L;
}
static const char *title_of(Window w) {
    static char b[256]; XClassHint ch; const char *s = name_of(w);
    if (*s) return s;
    if (XGetClassHint(dpy, w, &ch)) {
        snprintf(b, sizeof b, "%s", ch.res_class ? ch.res_class : "app");
        if (ch.res_name) XFree(ch.res_name);
        if (ch.res_class) XFree(ch.res_class);
        return b;
    }
    return "untitled";
}
static int barw(int i) { struct cl *c = &cs[i]; int w = tw(title_of(c->w)) + 28; if (w < 56) w = 56; return w > c->W ? c->W : w; }
static void bar_draw(int i) {
    struct cl *c = &cs[i]; const char *s = title_of(c->w); int w = barw(i);
    XSetWindowBackground(dpy, c->t, c->w == focus_w ? pfocus : pidle);
    XClearWindow(dpy, c->t);
    txt(c->t, (w - tw(s)) / 2, (bh - fa - fd) / 2 + fa, s, ptext);
}
static void bar_show(int i) {
    struct cl *c = &cs[i];
    if (c->on && c->mode != 2) { XMoveResizeWindow(dpy, c->t, c->x + c->W - barw(i), c->y - bh, barw(i), bh); XMapWindow(dpy, c->t); bar_draw(i); }
    else XUnmapWindow(dpy, c->t);
}
static void bar_move(int i) { struct cl *c = &cs[i]; if (c->on && c->mode != 2) XMoveResizeWindow(dpy, c->t, c->x + c->W - barw(i), c->y - bh, barw(i), bh); }
static void place(int i) { struct cl *c = &cs[i]; XMoveResizeWindow(dpy, c->w, c->x, c->y, c->W, c->H); bar_move(i); }
static void focus(int i) {
    Window prev = focus_w;
    if (i >= 0 && i < ncl && cs[i].on) {
        struct cl t = cs[i];
        if (t.w == prev) return;
        memmove(&cs[i], &cs[i + 1], (size_t)(ncl - i - 1) * sizeof t);
        cs[ncl - 1] = t;
        focus_w = t.w;
        if (!rb_on) XSetInputFocus(dpy, focus_w, RevertToPointerRoot, CurrentTime);
        XRaiseWindow(dpy, t.w); XRaiseWindow(dpy, t.t);
        if (rb_on) XRaiseWindow(dpy, rbw);
        if (mn_on) XRaiseWindow(dpy, mnw);
        if (al_on) XRaiseWindow(dpy, alw);
        if (t.mode == 2) XLowerWindow(dpy, dockw); else if (ndk) XRaiseWindow(dpy, dockw);
        if (prev) { int p = wi(prev); if (p >= 0) bar_show(p); }
        bar_show(ncl - 1);
        return;
    }
    if (focus_w) { int p = wi(focus_w); focus_w = 0; if (p >= 0) bar_show(p); }
    if (!rb_on) XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
}
static void save(int i) {
    struct cl *c = &cs[i];
    long s[11] = { c->mode, c->ws, c->min, c->x, c->y, c->W, c->H, c->sv[0], c->sv[1], c->sv[2], c->sv[3] };
    XChangeProperty(dpy, c->w, a_st, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)s, 11);
}
static void vis_update(int i) {
    struct cl *c = &cs[i]; int want = c->ws == cur && !c->min;
    if (want && !c->on) { c->on = 1; XMapWindow(dpy, c->w); set_st(c->w, NormalState); }
    else if (!want && c->on) { c->on = 0; c->pend++; XUnmapWindow(dpy, c->w); set_st(c->w, IconicState); }
    bar_show(i);
}
static int busy(int x, int y, int W, int H) {
    int i;
    for (i = 0; i < ncl; i++) {
        struct cl *c = &cs[i];
        if (c->ws == cur && !c->min && c->mode != 2 && x < c->x + c->W && c->x < x + W && y - bh < c->y + c->H && c->y - bh < y + H) return 1;
    }
    return 0;
}
static void place_new(struct cl *c) {
    int r, k, x, y, m = 0, j;
    for (r = 0; ; r++) {
        y = bh + r * (win_h + bh + gap);
        if (y + win_h > SH) break;
        for (k = 0; ; k++) {
            x = k * (win_w + gap);
            if (x + c->W > SW - dw) break;
            if (!busy(x, y, c->W, c->H)) { c->x = x; c->y = y; return; }
        }
    }
    for (j = 0; j < ncl; j++) if (cs[j].ws == cur) m++;
    c->x = (m % 10) * 32; c->y = bh + (m % 10) * 32;
}
static void snap(Window w, int *W, int *H) {
    XSizeHints h; long s; int b0, b1;
    if (!XGetWMNormalHints(dpy, w, &h, &s) || !(h.flags & PResizeInc)) return;
    b0 = (h.flags & PBaseSize) ? h.base_width : 0;
    b1 = (h.flags & PBaseSize) ? h.base_height : 0;
    if (h.width_inc > 1 && *W > b0) *W = b0 + (*W - b0) / h.width_inc * h.width_inc;
    if (h.height_inc > 1 && *H > b1) *H = b1 + (*H - b1) / h.height_inc * h.height_inc;
}
static void manage(Window w, int adopt) {
    XWindowAttributes a; struct cl *c; Atom t; int fmt; unsigned long n, l; unsigned char *p = 0;
    if (ncl >= MAXW || !XGetWindowAttributes(dpy, w, &a) || a.override_redirect) return;
    c = &cs[ncl]; memset(c, 0, sizeof *c);
    c->w = w; c->ws = cur; c->x = a.x; c->y = a.y; c->W = a.width; c->H = a.height;
    if (XGetWindowProperty(dpy, w, a_st, 0, 11, False, XA_CARDINAL, &t, &fmt, &n, &l, &p) == Success && p && n >= 11 && fmt == 32) {
        long *s = (long *)p;
        c->mode = s[0]; c->ws = s[1]; c->min = s[2]; c->x = s[3]; c->y = s[4]; c->W = s[5]; c->H = s[6];
        c->sv[0] = s[7]; c->sv[1] = s[8]; c->sv[2] = s[9]; c->sv[3] = s[10];
    }
    if (p) XFree(p);
    if (!adopt && !c->mode) {
        c->W = a.width > win_w ? win_w : a.width; c->H = a.height > win_h ? win_h : a.height;
        if (c->W < 20) c->W = 20;
        if (c->H < 20) c->H = 20;
        snap(w, &c->W, &c->H);
        place_new(c);
        c->settle = now() + 4000;
    }
    if (c->y < bh && !c->mode) c->y = bh;
    if ((c->ws != cur || c->min) && a.map_state == IsViewable) { c->pend++; XUnmapWindow(dpy, w); }
    c->t = XCreateSimpleWindow(dpy, root, 0, 0, 1, bh, 0, 0, pidle);
    own_mark(c->t);
    XSelectInput(dpy, c->t, ExposureMask | ButtonPressMask);
    XSelectInput(dpy, w, PropertyChangeMask);
    XSetWindowBorderWidth(dpy, w, 0);
    XGrabButton(dpy, AnyButton, AnyModifier, w, False, ButtonPressMask, GrabModeSync, GrabModeAsync, None, None);
    ncl++;
    place(ncl - 1);
    vis_update(ncl - 1);
    if (!adopt) focus(ncl - 1);
}
static void setmode(int i, int m) {
    struct cl *c = &cs[i]; Atom f = a_fs;
    if (c->mode == m) { c->x = c->sv[0]; c->y = c->sv[1]; c->W = c->sv[2]; c->H = c->sv[3]; c->mode = 0; }
    else {
        if (!c->mode) { c->sv[0] = c->x; c->sv[1] = c->y; c->sv[2] = c->W; c->sv[3] = c->H; }
        c->mode = m;
        if (m == 2) { c->x = 0; c->y = 0; c->W = SW; c->H = SH; }
        else { c->x = 0; c->y = bh; c->W = SW - dw; c->H = SH - bh; }
    }
    if (c->mode == 2) XChangeProperty(dpy, c->w, a_nst, XA_ATOM, 32, PropModeReplace, (unsigned char *)&f, 1);
    else XDeleteProperty(dpy, c->w, a_nst);
    place(i); bar_show(i); save(i); focus(i);
}
static void closew(int i) {
    Window w = cs[i].w; Atom *l = 0; int n = 0, k, p = 0;
    if (XGetWMProtocols(dpy, w, &l, &n)) { for (k = 0; k < n; k++) if (l[k] == a_del) p = 1; if (l) XFree(l); }
    if (p) {
        XEvent e; memset(&e, 0, sizeof e);
        e.xclient.type = ClientMessage; e.xclient.window = w; e.xclient.message_type = a_proto; e.xclient.format = 32;
        e.xclient.data.l[0] = a_del; e.xclient.data.l[1] = CurrentTime;
        XSendEvent(dpy, w, False, NoEventMask, &e);
    } else XKillClient(dpy, w);
}
static void unmanage(int i) {
    int was = cs[i].w == focus_w;
    XDestroyWindow(dpy, cs[i].t);
    memmove(&cs[i], &cs[i + 1], (size_t)(ncl - i - 1) * sizeof cs[0]);
    ncl--;
    if (was) { focus_w = 0; focus(top_on()); }
}
static void minimize(int i) { int was = cs[i].w == focus_w; cs[i].min = 1; vis_update(i); if (was) focus(top_on()); }
static void unminimize(void) { int i; for (i = 0; i < ncl; i++) cs[i].min = 0; for (i = 0; i < ncl; i++) vis_update(i); focus(top_on()); }
static void switch_ws(int n) { int i; if (n == cur) return; cur = n; for (i = 0; i < ncl; i++) vis_update(i); focus(top_on()); }
static void next_win(void) { int i; for (i = 0; i < ncl; i++) if (cs[i].ws == cur && !cs[i].min) { focus(i); return; } }
static void prev_win(void) {
    int i = top_on(); struct cl t;
    if (i < 0) return;
    t = cs[i];
    memmove(&cs[1], &cs[0], (size_t)i * sizeof t);
    cs[0] = t;
    XLowerWindow(dpy, t.w); XLowerWindow(dpy, t.t);
    focus(top_on());
}
static void drag_begin(int k, Window w, int x, int y) {
    int i = wi(w);
    if (i < 0 || cs[i].mode == 2) return;
    if (XGrabPointer(dpy, root, False, ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime) != GrabSuccess) return;
    dragging = k; dragw = w; dx0 = x; dy0 = y;
    dox = cs[i].x; doy = cs[i].y; dow = cs[i].W; doh = cs[i].H;
}
static void drag_move(int x, int y) {
    int i = wi(dragw), dx = x - dx0, dy = y - dy0;
    if (i < 0) { dragging = 0; XUngrabPointer(dpy, CurrentTime); return; }
    if (dragging == 1) { cs[i].x = dox + dx; cs[i].y = doy + dy; }
    else {
        cs[i].W = dow + dx < 60 ? 60 : dow + dx;
        cs[i].H = doh + dy < 40 ? 40 : doh + dy;
        snap(cs[i].w, &cs[i].W, &cs[i].H);
    }
    place(i);
}
static void dock_layout(void) {
    int per = SH / tile, i, n = ndk;
    if (per < 1) per = 1;
    if (!n) { dw = 0; XUnmapWindow(dpy, dockw); return; }
    dw = ((n + per - 1) / per) * tile;
    XMoveResizeWindow(dpy, dockw, SW - dw, 0, dw, (n < per ? n : per) * tile);
    for (i = 0; i < n; i++) {
        int c = i / per, r = i % per;
        XMoveResizeWindow(dpy, ds[i].w, dw - (c + 1) * tile, r * tile, tile, tile);
        XMapWindow(dpy, ds[i].w);
    }
    XMapWindow(dpy, dockw); XRaiseWindow(dpy, dockw);
}
static int dk_add(int kind, Window w) {
    if (ndk == dcap) { dcap = dcap ? dcap * 2 : 16; ds = realloc(ds, (size_t)dcap * sizeof *ds); if (!ds) die("out of memory"); }
    memset(&ds[ndk], 0, sizeof *ds);
    ds[ndk].kind = kind; ds[ndk].w = w;
    return ndk++;
}
static void launch_draw(int d) {
    char lab[10]; const char *c = ds[d].cmd; int i;
    for (i = 0; i < 9 && c[i] && c[i] != ' '; i++) lab[i] = c[i];
    lab[i] = 0;
    XClearWindow(dpy, ds[d].w);
    XSetForeground(dpy, gc, ptext);
    XDrawRectangle(dpy, ds[d].w, gc, 3, 3, tile - 7, tile - 7);
    txt(ds[d].w, (tile - tw(lab)) / 2, tile / 2 + fa / 2, lab, ptext);
}
static void launch_add(const char *img, const char *cmd) {
    Window w = XCreateSimpleWindow(dpy, dockw, 0, 0, tile, tile, 0, pink, pdock);
    Pixmap pm = 0; int k;
    XSelectInput(dpy, w, ExposureMask | ButtonPressMask);
    k = dk_add(DK_LAUNCH, w);
    snprintf(ds[k].cmd, sizeof ds[k].cmd, "%s", cmd);
    if (img && *img) {
        if ((pm = pic(img, tile, tile))) { XSetWindowBackgroundPixmap(dpy, w, pm); ds[k].pic = 1; }
        else fprintf(stderr, "wm: cannot read icon %s\n", img);
    }
    dock_layout();
}
static void stats_add(void) {
    statw = XCreateSimpleWindow(dpy, dockw, 0, 0, tile, tile, 0, pink, pdock);
    XSelectInput(dpy, statw, ExposureMask | ButtonPressMask);
    statpm = XCreatePixmap(dpy, statw, tile, tile, depth);
    dk_add(DK_STATS, statw);
}
static void dock_load(void) {
    FILE *f = fopen(dock_path, "r"); char l[4096], *c;
    if (!f) return;
    while (fgets(l, sizeof l, f)) {
        l[strcspn(l, "\n")] = 0;
        if (l[0] == '#') continue;
        c = strchr(l, '\t');
        if (!c) c = strchr(l, '|');
        if (!c) continue;
        *c++ = 0;
        if (*trim(c)) launch_add(trim(l), trim(c));
    }
    fclose(f);
}
static void release(void) {
    int i; long wsn = cur;
    for (i = 0; i < ncl; i++) save(i);
    XChangeProperty(dpy, root, a_cur, XA_CARDINAL, 32, PropModeReplace, (unsigned char *)&wsn, 1);
    XSync(dpy, False);
}
static void rb_draw(void) {
    static const char *t[] = { "run", "dock command", "dock picture" };
    int s = 0;
    XSetWindowBackground(dpy, rbw, pdock); XClearWindow(dpy, rbw);
    XSetForeground(dpy, gc, ppanel); XFillRectangle(dpy, rbw, gc, 0, 0, box, bh);
    txt(rbw, 6, (bh - fa - fd) / 2 + fa, t[rbm], pink);
    while (s < rbl && tw(rbt + s) + tw("_") > box - 12) s++;
    txt(rbw, 6, bh + fa + 8, rbt + s, ptext);
    txt(rbw, 6 + tw(rbt + s), bh + fa + 8, "_", ptext);
}
static void rb_open(int m) {
    int x, y, wx, wy; Window r, c; unsigned mk;
    if (rb_on) return;
    XQueryPointer(dpy, root, &r, &c, &x, &y, &wx, &wy, &mk);
    if (x + box > SW) x = SW - box;
    if (y + box > SH) y = SH - box;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    rbm = m; rbl = 0; rbt[0] = 0; rb_on = 1;
    XMoveResizeWindow(dpy, rbw, x, y, box, box);
    XMapRaised(dpy, rbw);
    XSetInputFocus(dpy, rbw, RevertToPointerRoot, CurrentTime);
    rb_draw();
}
static void rb_close(void) { if (!rb_on) return; rb_on = 0; XUnmapWindow(dpy, rbw); focus(top_on()); }
static Atom ptg(int k) { return k == 0 ? a_utf8 : k == 1 ? XA_STRING : a_uri; }
static void paste_ask(Atom sel) { pst = 0; psel = sel; XConvertSelection(dpy, sel, ptg(0), a_paste, rbw, CurrentTime); XFlush(dpy); }
static void paste_uri(const unsigned char *p, unsigned long n) {
    unsigned long i = 0; char h[3] = { 0 };
    if (n > 7 && !strncmp((const char *)p, "file://", 7)) i = 7;
    for (; i < n && p[i] != '\r' && p[i] != '\n'; i++) {
        if (p[i] == '%' && i + 2 < n) { h[0] = (char)p[i + 1]; h[1] = (char)p[i + 2]; i += 2; if (rbl < (int)sizeof rbt - 1) rbt[rbl++] = (char)strtol(h, 0, 16); }
        else if (rbl < (int)sizeof rbt - 1) rbt[rbl++] = (char)p[i];
    }
    rbt[rbl] = 0;
}
static void paste_add(const unsigned char *p, unsigned long n) {
    unsigned long i;
    for (i = 0; i < n; i++) if (p[i] >= 32 && p[i] < 127 && rbl < (int)sizeof rbt - 1) { rbt[rbl++] = (char)p[i]; rbt[rbl] = 0; }
}
static void on_sel(XSelectionEvent *e) {
    Atom t; int fmt; unsigned long n = 0, l; unsigned char *p = 0;
    if (e->property == None) {
        if (pst < 2) { pst++; XConvertSelection(dpy, psel, ptg(pst), a_paste, rbw, CurrentTime); XFlush(dpy); }
        return;
    }
    if (XGetWindowProperty(dpy, rbw, a_paste, 0, 1 << 18, True, AnyPropertyType, &t, &fmt, &n, &l, &p) != Success) return;
    if (t == a_incr) incr = 1;
    else if (p && fmt == 8) { if (pst == 2) paste_uri(p, n); else paste_add(p, n); }
    if (p) XFree(p);
    if (rb_on) rb_draw();
}
static void on_incr(XPropertyEvent *e) {
    Atom t; int fmt; unsigned long n = 0, l; unsigned char *p = 0;
    if (!incr || e->window != rbw || e->atom != a_paste || e->state != PropertyNewValue) return;
    if (XGetWindowProperty(dpy, rbw, a_paste, 0, 1 << 18, True, AnyPropertyType, &t, &fmt, &n, &l, &p) != Success) return;
    if (!n) incr = 0;
    else if (p && fmt == 8) paste_add(p, n);
    if (p) XFree(p);
    if (rb_on) rb_draw();
}
static void rb_key(XKeyEvent *e) {
    char b[32]; KeySym s = 0; int i, n = XLookupString(e, b, sizeof b, &s, 0);
    if (s == XK_Escape) { rb_close(); return; }
    if ((s == XK_Insert && (e->state & ShiftMask)) || ((e->state & ControlMask) && (s == XK_v || s == XK_V))) { paste_ask(a_clip); return; }
    if (s == XK_Return || s == XK_KP_Enter) {
        static char c[8192]; snprintf(c, sizeof c, "%s", rbt);
        if (rbm == 0) { rb_close(); spawn(c); }
        else if (rbm == 1) {
            if (!c[0]) { rb_close(); return; }
            snprintf(rbcmd, sizeof rbcmd, "%s", c);
            rbm = 2; rbl = 0; rbt[0] = 0;
        } else { rb_close(); launch_add(c, rbcmd); dock_save(c, rbcmd); }
        if (rb_on) rb_draw();
        return;
    }
    if (s == XK_BackSpace) { if (rbl) rbt[--rbl] = 0; }
    else for (i = 0; i < n; i++) if (b[i] >= 32 && b[i] < 127 && rbl < (int)sizeof rbt - 1) { rbt[rbl++] = b[i]; rbt[rbl] = 0; }
    rb_draw();
}
static void mn_draw(void) {
    int j, y = bh;
    XSetWindowBackground(dpy, mnw, pdock); XClearWindow(dpy, mnw);
    XSetForeground(dpy, gc, ppanel); XFillRectangle(dpy, mnw, gc, 0, 0, mn_w, bh);
    txt(mnw, 6, (bh - fa - fd) / 2 + fa, "menu", pink);
    for (j = 0; j < NMENU; j++) {
        if (!menu[j].l) { XSetForeground(dpy, gc, ptext); XDrawLine(dpy, mnw, gc, 2, y + 2, mn_w - 3, y + 2); y += 5; continue; }
        if (j == mn_hi) { XSetForeground(dpy, gc, ppanel); XFillRectangle(dpy, mnw, gc, 1, y, mn_w - 2, ih); txt(mnw, 8, y + (ih - fa - fd) / 2 + fa, menu[j].l, pink); }
        else txt(mnw, 8, y + (ih - fa - fd) / 2 + fa, menu[j].l, ptext);
        y += ih;
    }
}
static int mn_at(int y) {
    int j, t = bh;
    if (y < bh) return -1;
    for (j = 0; j < NMENU; j++) {
        int h = menu[j].l ? ih : 5;
        if (y >= t && y < t + h) return menu[j].l ? j : -1;
        t += h;
    }
    return -1;
}
static void mn_open(int x, int y) {
    int j, h = bh;
    mn_w = tw("menu") + 16;
    for (j = 0; j < NMENU; j++) { if (menu[j].l) { int w = tw(menu[j].l) + 24; if (w > mn_w) mn_w = w; h += ih; } else h += 5; }
    if (x + mn_w > SW) x = SW - mn_w;
    if (y + h > SH) y = SH - h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    mn_on = 1; mn_hi = -1;
    XMoveResizeWindow(dpy, mnw, x, y, mn_w, h);
    XMapRaised(dpy, mnw);
    mn_draw();
}
static void mn_close(void) { if (mn_on) { mn_on = 0; XUnmapWindow(dpy, mnw); } }
static void clock_add(void) {
    clockw = XCreateSimpleWindow(dpy, dockw, 0, 0, tile, tile, 0, ptext, pdock);
    XSelectInput(dpy, clockw, ExposureMask | ButtonPressMask);
    dk_add(DK_CLOCK, clockw);
}
static int clip_add(const char *t, size_t n) {
    char *c = malloc(n + 1); int i;
    if (!c) return 0;
    memcpy(c, t, n); c[n] = 0;
    if (nh && !strcmp(clips[0], c)) { free(c); return 0; }
    if (nh == MAXCLIP) { free(clips[MAXCLIP - 1]); nh = MAXCLIP - 1; }
    for (i = nh; i > 0; i--) clips[i] = clips[i - 1];
    clips[0] = c; nh++; hidx = 0;
    return 1;
}
static void push_hist(const char *t, size_t n) {
    FILE *f;
    if (!clip_add(t, n)) return;
    f = fopen(clip_path, "ab");
    if (f) { fwrite(clips[0], 1, strlen(clips[0]) + 1, f); fclose(f); }
    if (clipt) clip_draw(clipt);
}
static void clip_load(void) {
    FILE *f = fopen(clip_path, "rb"); char *line = 0; size_t cap = 0; ssize_t len; int i;
    if (!f) return;
    while ((len = getdelim(&line, &cap, 0, f)) > 0) if (len > 1) clip_add(line, (size_t)len - 1);
    free(line); fclose(f);
    f = fopen(clip_path, "wb");
    if (!f) return;
    for (i = nh - 1; i >= 0; i--) fwrite(clips[i], 1, strlen(clips[i]) + 1, f);
    fclose(f);
}
static void own_hist(int k) {
    if (k < 0 || k >= nh) return;
    cur_text = clips[k];
    XSetSelectionOwner(dpy, a_clip, clipw, CurrentTime);
    XFlush(dpy);
}
static void fx_owner(XEvent *ev) {
    struct { int type; unsigned long serial; int send; Display *d; Window win; int sub; Window owner; Atom sel; } *e = (void *)ev;
    if (e->sel != a_clip || e->owner == clipw) return;
    if (!e->owner) { if (nh) own_hist(0); return; }
    cur_text = 0;
    XConvertSelection(dpy, a_clip, a_utf8, a_clipprop, clipw, CurrentTime);
    XFlush(dpy);
}
static void clip_got(XSelectionEvent *e) {
    Atom t; int fmt; unsigned long n = 0, l; unsigned char *p = 0;
    if (e->property == None) return;
    if (XGetWindowProperty(dpy, clipw, a_clipprop, 0, 1 << 18, True, AnyPropertyType, &t, &fmt, &n, &l, &p) == Success) {
        if (p && fmt == 8 && n && t != a_incr) push_hist((const char *)p, (size_t)n);
        if (p) XFree(p);
    }
}
static void serve_req(XSelectionRequestEvent *r) {
    XEvent n; long ts[3] = { (long)a_utf8, (long)XA_STRING, (long)a_targets }; int ok = 0;
    if (r->property == None) r->property = r->target;
    memset(&n, 0, sizeof n);
    n.xselection.type = SelectionNotify; n.xselection.requestor = r->requestor; n.xselection.selection = r->selection;
    n.xselection.target = r->target; n.xselection.time = r->time; n.xselection.property = None;
    if (r->target == a_targets) { XChangeProperty(dpy, r->requestor, r->property, XA_ATOM, 32, PropModeReplace, (unsigned char *)ts, 3); ok = 1; }
    else if ((r->target == a_utf8 || r->target == XA_STRING) && cur_text) {
        XChangeProperty(dpy, r->requestor, r->property, r->target, 8, PropModeReplace, (const unsigned char *)cur_text, (int)strlen(cur_text));
        ok = 1;
    }
    if (ok) n.xselection.property = r->property;
    XSendEvent(dpy, r->requestor, False, 0, &n);
    XFlush(dpy);
}
static void fx_init(void) {
    void *h = dlopen("libXfixes.so.3", RTLD_LAZY);
    int maj = 0, min = 0, ev = 0, er = 0;
    int (*qv)(Display *, int *, int *);
    int (*qe)(Display *, int *, int *);
    void (*sel)(Display *, Window, Atom, unsigned long);
    if (!h) return;
    qv = (int (*)(Display *, int *, int *))dlsym(h, "XFixesQueryVersion");
    qe = (int (*)(Display *, int *, int *))dlsym(h, "XFixesQueryExtension");
    sel = (void (*)(Display *, Window, Atom, unsigned long))dlsym(h, "XFixesSelectSelectionInput");
    if (!qv || !qe || !sel || !qv(dpy, &maj, &min) || !qe(dpy, &ev, &er)) return;
    fx_base = ev;
    sel(dpy, root, a_clip, 7UL);
}
static void tile_add(int kind, Window *w) {
    *w = XCreateSimpleWindow(dpy, dockw, 0, 0, tile, tile, 0, ptext, pdock);
    XSelectInput(dpy, *w, ExposureMask | ButtonPressMask);
    dk_add(kind, *w);
}
static void clip_step(int dir) {
    if (!nh) return;
    hidx = (hidx + dir + nh) % nh;
    own_hist(hidx);
    if (clipt) clip_draw(clipt);
}
static void clip_cycle(void) { clip_show = 1; clip_step(1); }
static void root_cursor(void) {
    void *h = dlopen("libXcursor.so.1", RTLD_LAZY);
    Cursor (*load)(Display *, const char *) = h ? (Cursor (*)(Display *, const char *))dlsym(h, "XcursorLibraryLoadCursor") : 0;
    Cursor c = load ? load(dpy, "left_ptr") : 0;
    if (!c) c = XCreateFontCursor(dpy, XC_left_ptr);
    XDefineCursor(dpy, root, c);
}
static void restart(void) {
    const char *s = getenv("PLAINWM_SRC");
    char *av[3];
    release();
    if (s) execl("/bin/sh", "sh", s, "--restart", (char *)0);
    av[0] = argv_saved[0]; av[1] = "--restart"; av[2] = 0;
    execv("/proc/self/exe", av);
    die("restart failed");
}
static void act(int a, int n, const char *c) {
    int x, y, i; Window r, ch; int wx, wy; unsigned m;
    switch (a) {
    case SPAWN: spawn(c); break;
    case RUN: rb_open(0); break;
    case MKDOCK: rb_open(1); break;
    case MENU: XQueryPointer(dpy, root, &r, &ch, &x, &y, &wx, &wy, &m); mn_open(x, y); break;
    case CLOSE: i = wi(focus_w); if (i >= 0) closew(i); break;
    case MAXI: i = wi(focus_w); if (i >= 0) setmode(i, 1); break;
    case FULL: i = wi(focus_w); if (i >= 0) setmode(i, 2); break;
    case MIN: i = wi(focus_w); if (i >= 0) minimize(i); break;
    case UNMIN: unminimize(); break;
    case NEXT: next_win(); break;
    case PREV: prev_win(); break;
    case WS: switch_ws(n); break;
    case BRIGHT: bright(n); break;
    case RESTART: restart(); break;
    case CLIP: clip_cycle(); break;
    case QUIT: release(); exit(0);
    default: break;
    }
}
static void grab(void) {
    static const unsigned ex[4] = { 0, LockMask, Mod2Mask, LockMask | Mod2Mask };
    int k, j;
    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    for (k = 0; k < NKEYS; k++) {
        kc[k] = XKeysymToKeycode(dpy, XStringToKeysym(keys[k].k));
        for (j = 0; kc[k] && j < 4; j++) XGrabKey(dpy, kc[k], keys[k].m | ex[j], root, False, GrabModeAsync, GrabModeAsync);
    }
}
static void key(XKeyEvent *e) {
    unsigned s = e->state & ~(LockMask | Mod2Mask); int k;
    if (rb_on && e->window == rbw) { rb_key(e); return; }
    for (k = 0; k < NKEYS; k++) if (kc[k] == e->keycode && keys[k].m == s) { act(keys[k].a, keys[k].n, keys[k].c); return; }
}
static void button(XButtonEvent *e) {
    unsigned s = e->state & ~(LockMask | Mod2Mask); int i, d, j, win = wi(e->window);
    if (mn_on) {
        j = mn_at(e->y);
        if (e->window == mnw && j >= 0) { mn_close(); act(menu[j].a, 0, menu[j].c); }
        else if (e->window != mnw) mn_close();
        if (win >= 0) XAllowEvents(dpy, ReplayPointer, CurrentTime);
        return;
    }
    if (e->window == rbw) { if (e->button == Button2) paste_ask(a_prim); return; }
    if (e->window == alw) { alert_show(0); al_dis = 1; return; }
    if (e->window == root) {
        if (s) return;
        if (e->button == btn_run) { if (rb_on) rb_close(); else rb_open(0); }
        else if (e->button == btn_menu) mn_open(e->x_root, e->y_root);
        return;
    }
    if ((d = di(e->window)) >= 0) {
        switch (ds[d].kind) {
        case DK_LAUNCH: spawn(ds[d].cmd); break;
        case DK_VOL:
            if (e->button == 4 || e->button == 6) vol_set(2);
            else if (e->button == 5 || e->button == 7) vol_set(-2);
            else if (e->button == 1 || e->button == 3) vol_toggle();
            break;
        case DK_CLIP: clip_show = 1; clip_step(e->button == 4 ? -1 : 1); break;
        default: tick(); break;
        }
        return;
    }
    if ((i = ti(e->window)) >= 0) {
        Window w = cs[i].w;
        focus(i);
        if (e->button == btn_move) drag_begin(1, w, e->x_root, e->y_root);
        return;
    }
    if (win < 0) return;
    {
        Window w = e->window; int dr = (s & WINMOD) && (e->button == btn_move || e->button == btn_resize);
        focus(wi(w));
        if (dr) { XAllowEvents(dpy, AsyncPointer, CurrentTime); drag_begin(e->button == btn_move ? 1 : 2, w, e->x_root, e->y_root); }
        else XAllowEvents(dpy, ReplayPointer, CurrentTime);
    }
}
static void expose(XExposeEvent *e) {
    int i, d;
    if (e->count) return;
    if (e->window == rbw) rb_draw();
    else if (e->window == mnw) mn_draw();
    else if (e->window == alw) { if (al_on) alert_draw(); }
    else if ((i = ti(e->window)) >= 0) bar_draw(i);
    else if ((d = di(e->window)) >= 0) {
        switch (ds[d].kind) {
        case DK_LAUNCH: if (!ds[d].pic) launch_draw(d); break;
        case DK_STATS: stats_draw(ds[d].w); break;
        case DK_CLOCK: lastc[0] = 0; clock_draw(ds[d].w); break;
        case DK_WATT: lastw[0] = 0; watt_draw(ds[d].w); break;
        case DK_VOL: vol_draw(ds[d].w); break;
        case DK_CLIP: clip_draw(ds[d].w); break;
        }
    }
}
static void configure(XConfigureRequestEvent *r) {
    int i = wi(r->window); XWindowChanges c;
    if (i < 0) {
        c.x = r->x; c.y = r->y; c.width = r->width; c.height = r->height;
        c.border_width = r->border_width; c.sibling = r->above; c.stack_mode = r->detail;
        XConfigureWindow(dpy, r->window, r->value_mask, &c);
        return;
    }
    if (!cs[i].mode && now() >= cs[i].settle) {
        if (r->value_mask & CWWidth) cs[i].W = r->width < 20 ? 20 : r->width;
        if (r->value_mask & CWHeight) cs[i].H = r->height < 20 ? 20 : r->height;
    }
    place(i);
}
static void adopt_all(void) {
    Window r, p, *k = 0; unsigned n = 0, i; XWindowAttributes a;
    if (!XQueryTree(dpy, root, &r, &p, &k, &n)) return;
    for (i = 0; i < n; i++) {
        Window w = k[i]; Atom t; int fmt, hid = 0; unsigned long c, l; unsigned char *pr = 0;
        if (own(w) || !XGetWindowAttributes(dpy, w, &a)) continue;
        if (XGetWindowProperty(dpy, w, a_own, 0, 1, False, XA_CARDINAL, &t, &fmt, &c, &l, &pr) == Success && pr) { XFree(pr); XDestroyWindow(dpy, w); continue; }
        if (pr) XFree(pr);
        if (XGetWindowProperty(dpy, w, a_state, 0, 1, False, a_state, &t, &fmt, &c, &l, &pr) == Success && pr) { hid = c && *(long *)pr == IconicState; XFree(pr); }
        if (a.override_redirect || (a.map_state != IsViewable && !hid)) continue;
        manage(w, 1);
    }
    if (k) XFree(k);
}
static void on_event(XEvent *ev) {
    if (fx_base && ev->type == fx_base) { fx_owner(ev); return; }
    if (ev->type == KeyPress || ev->type == ButtonPress || ev->type == MotionNotify) {
        lastact = now();
        if (asleep) { wake(); return; }
    }
    switch (ev->type) {
    case MapRequest: {
        Window w = ev->xmaprequest.window; int i = wi(w);
        if (i >= 0) vis_update(i);
        else if (!own(w)) manage(w, 0);
        break;
    }
    case ConfigureRequest: configure(&ev->xconfigurerequest); break;
    case UnmapNotify: {
        int i = wi(ev->xunmap.window);
        if (i >= 0) { if (cs[i].pend) cs[i].pend--; else unmanage(i); }
        break;
    }
    case DestroyNotify: { int i = wi(ev->xdestroywindow.window); if (i >= 0) unmanage(i); break; }
    case SelectionRequest: if (ev->xselectionrequest.owner == clipw) serve_req(&ev->xselectionrequest); break;
    case ButtonPress: button(&ev->xbutton); break;
    case ButtonRelease:
        if (dragging) { int i = wi(dragw); dragging = 0; XUngrabPointer(dpy, CurrentTime); if (i >= 0) save(i); }
        break;
    case MotionNotify:
        if (dragging) drag_move(ev->xmotion.x_root, ev->xmotion.y_root);
        else if (mn_on && ev->xmotion.window == mnw) {
            int j = mn_at(ev->xmotion.y);
            if (j != mn_hi) { mn_hi = j; mn_draw(); }
        }
        break;
    case KeyPress: key(&ev->xkey); break;
    case Expose: expose(&ev->xexpose); break;
    case SelectionNotify: if (ev->xselection.requestor == clipw) clip_got(&ev->xselection); else on_sel(&ev->xselection); break;
    case PropertyNotify: {
        int i;
        if (incr && ev->xproperty.window == rbw) { on_incr(&ev->xproperty); break; }
        i = wi(ev->xproperty.window);
        if (i >= 0 && cs[i].on && (ev->xproperty.atom == XA_WM_NAME || ev->xproperty.atom == a_nname)) bar_show(i);
        break;
    }
    case ClientMessage:
        if (ev->xclient.message_type == a_nst) {
            int i = wi(ev->xclient.window); long *d = ev->xclient.data.l;
            if (i >= 0 && ((Atom)d[1] == a_fs || (Atom)d[2] == a_fs)) {
                int on = cs[i].mode == 2;
                int want = d[0] == 2 ? !on : d[0] == 1;
                if (want != on) setmode(i, 2);
            }
        }
        break;
    case MappingNotify: XRefreshKeyboardMapping(&ev->xmapping); grab(); break;
    default: break;
    }
}
int main(int argc, char **argv) {
    XEvent ev; int i, nb, restarted = argc > 1 && !strcmp(argv[1], "--restart");
    unsigned char pm[32]; Atom sup[2]; Pixmap wp; const char *h = getenv("HOME");
    argv_saved = argv;
    signal(SIGCHLD, SIG_IGN);
    dpy = XOpenDisplay(NULL);
    if (!dpy) die("cannot open display (is DISPLAY set?)");
    scr = DefaultScreen(dpy); root = RootWindow(dpy, scr);
    SW = DisplayWidth(dpy, scr); SH = DisplayHeight(dpy, scr);
    depth = DefaultDepth(dpy, scr); vis = DefaultVisual(dpy, scr);
    XSetErrorHandler(xerr);
    XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | ButtonPressMask);
    XSync(dpy, False);
    if (other) die("another window manager is running");
    fcntl(ConnectionNumber(dpy), F_SETFD, FD_CLOEXEC);
    a_state = at("WM_STATE"); a_proto = at("WM_PROTOCOLS"); a_del = at("WM_DELETE_WINDOW");
    a_own = at("WM_OWN_MARK"); a_st = at("WM_MANAGED_STATE");
    a_nst = at("_NET_WM_STATE"); a_fs = at("_NET_WM_STATE_FULLSCREEN"); a_clip = at("CLIPBOARD");
    a_cur = at("WM_CUR_WS"); a_uri = at("text/uri-list"); a_clipprop = at("WM_CLIP"); a_targets = at("TARGETS"); a_nname = at("_NET_WM_NAME"); a_utf8 = at("UTF8_STRING"); a_incr = at("INCR"); a_paste = at("WM_PASTE_DATA"); a_prim = XA_PRIMARY;
    nb = XGetPointerMapping(dpy, pm, sizeof pm);
    if (nb >= 3) { memcpy(pm, pmap, 3); XSetPointerMapping(dpy, pm, nb); }
    fnt = XLoadQueryFont(dpy, font);
    if (!fnt) fnt = XLoadQueryFont(dpy, "fixed");
    if (!fnt) die("no usable font");
    sfnt = XLoadQueryFont(dpy, "5x8");
    if (!sfnt) sfnt = fnt;
    fa = fnt->ascent; fd = fnt->descent; th = fa + fd + 4; bh = th + bar_extra; ih = fa + fd + 6;
    pdesk = px(c_desktop); pfocus = px(c_bar_focus); pidle = px(c_bar_idle); ptext = px(c_text);
    pdock = px(c_dock); pgraph = px(c_graph); ppanel = px(c_panel); pink = px(c_ink);
    gc = XCreateGC(dpy, root, 0, 0);
    XSetFont(dpy, gc, fnt->fid);
    XSetWindowBackground(dpy, root, pdesk);
    XClearWindow(dpy, root);
    root_cursor();
    wp = pic(wallpaper, SW, SH);
    if (wp) { XSetWindowBackgroundPixmap(dpy, root, wp); XClearWindow(dpy, root); }
    dockw = XCreateSimpleWindow(dpy, root, SW - tile, 0, tile, tile, 0, 0, 0);
    XSetWindowBackgroundPixmap(dpy, dockw, ParentRelative);
    XSelectInput(dpy, dockw, SubstructureNotifyMask);
    rbw = XCreateSimpleWindow(dpy, root, 0, 0, box, box, 0, 0, pdock);
    XSelectInput(dpy, rbw, KeyPressMask | ExposureMask | ButtonPressMask | PropertyChangeMask);
    mnw = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, pdock);
    XSelectInput(dpy, mnw, ExposureMask | ButtonPressMask | PointerMotionMask);
    alw = XCreateSimpleWindow(dpy, root, (SW - box) / 2, margin, box, 40, 0, 0, ppanel);
    XSelectInput(dpy, alw, ExposureMask | ButtonPressMask);
    own_mark(dockw); own_mark(rbw); own_mark(mnw); own_mark(alw);
    sup[0] = a_nst; sup[1] = a_fs;
    XChangeProperty(dpy, root, at("_NET_SUPPORTED"), XA_ATOM, 32, PropModeReplace, (unsigned char *)sup, 2);
    XChangeProperty(dpy, root, at("_NET_SUPPORTING_WM_CHECK"), XA_WINDOW, 32, PropModeReplace, (unsigned char *)&dockw, 1);
    clipw = XCreateSimpleWindow(dpy, root, -10, -10, 1, 1, 0, 0, 0);
    fx_init();
    snprintf(clip_path, sizeof clip_path, "%s/.wmclip", h ? h : ".");
    clip_load();
    if (nh) own_hist(0);
    if (*battery) snprintf(bat_dir, sizeof bat_dir, "%s", battery);
    else first("/sys/class/power_supply", "BAT", bat_dir, sizeof bat_dir);
    if (*backlight) snprintf(bl_dir, sizeof bl_dir, "%s", backlight);
    else first("/sys/class/backlight", "", bl_dir, sizeof bl_dir);
    snprintf(dock_path, sizeof dock_path, "%s/.wmdock", h ? h : ".");
    if (access(dock_path, F_OK)) { FILE *f = fopen(dock_path, "w"); if (f) { fputs("# picture<TAB>command   (alt+shift+r applies changes)\n", f); fclose(f); } }
    stats_add();
    clock_add();
    tile_add(DK_WATT, &wattw);
    tile_add(DK_VOL, &volt);
    tile_add(DK_CLIP, &clipt);
    { FILE *f = popen("pactl info 2>/dev/null", "r"); char b[512]; if (f) { while (fgets(b, sizeof b, f)) if (!strncmp(b, "Default Sink: ", 14)) { b[strcspn(b, "\n")] = 0; snprintf(dsink, sizeof dsink, "%s", b + 14); pulse = 1; } pclose(f); } }
    vol_read();
    dock_load();
    dock_layout();
    grab();
    {
        Atom t; int fmt; unsigned long n = 0, l; unsigned char *p = 0;
        if (XGetWindowProperty(dpy, root, a_cur, 0, 1, False, XA_CARDINAL, &t, &fmt, &n, &l, &p) == Success && p && n == 1) cur = (int)*(long *)p;
        if (p) XFree(p);
        if (cur < 0 || cur > 3) cur = 0;
    }
    adopt_all();
    dock_layout();
    if (!restarted) for (i = 0; autostart[i]; i++) spawn(autostart[i]);
    lastact = now();
    tick();
    for (;;) {
        while (XPending(dpy)) { XNextEvent(dpy, &ev); on_event(&ev); }
        XFlush(dpy);
        {
            fd_set f; struct timeval tv; long w; int cf = ConnectionNumber(dpy);
            FD_ZERO(&f); FD_SET(cf, &f);
            w = nexttick - now(); if (w < 0) w = 0;
            tv.tv_sec = w / 1000; tv.tv_usec = (w % 1000) * 1000;
            select(cf + 1, &f, 0, 0, stats_s && !asleep ? &tv : 0);
        }
        if (stats_s && !asleep && now() >= nexttick) tick();
    }
}
