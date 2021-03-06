#ifndef OUTCURSES_OUTCURSES
#define OUTCURSES_OUTCURSES

#include <panel.h>
#include <ncurses.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Receive a pointer to a version string of the form MAJOR.MINOR.PATCH.
const char* outcurses_version(void);

// Initialize the library. Will initialize ncurses suitably for fullscreen mode
// unless initcurses is false. You are recommended to call outcurses_init prior
// to interacting with ncurses, and to set initcurses to true.
WINDOW* outcurses_init(bool initcurses);

// Stop the library. If stopcurses is true, endwin() will be called, (ideally)
// restoring the screen and cleaning up ncurses.
int outcurses_stop(bool stopcurses);

// A set of RGB color components
typedef struct outcurses_rgb {
  int r, g, b;
} outcurses_rgb;

// Do a palette fade on the specified screen over the course of ms milliseconds.
// FIXME allow colorpair range to be specified for partial-screen fades
int fadeout(WINDOW* w, unsigned ms);

// count ought be COLORS to retrieve the entire palette. palette is a
// count-element array of rgb values. if maxes is non-NULL, it points to a
// single rgb triad, which will be filled in with the maximum components found.
// if zeroout is set, the palette will be set to all 0s. to fade in, we
// generally want to prepare the screen using a zerod-out palette, then
// initiate the fadein targeting the true palette.
int retrieve_palette(int count, outcurses_rgb* palette, outcurses_rgb* maxes,
                     bool zeroout);

// fade in to the specified palette
// FIXME can we not just generalize this plus fadeout to 'fadeto'?
int fadein(WINDOW* w, int count, const outcurses_rgb* palette, unsigned ms);

// Restores a palette through count colors.
int set_palette(int count, const outcurses_rgb* palette);

// A panelreel is an ncurses window devoted to displaying zero or more
// line-oriented, contained panels between which the user may navigate. If at
// least one panel exists, there is an active panel. As much of the active
// panel as is possible is always displayed. If there is space left over, other
// panels are included in the display. Panels can come and go at any time, and
// can grow or shrink at any time.
//
// This structure is amenable to line- and page-based navigation via keystrokes,
// scrolling gestures, trackballs, scrollwheels, touchpads, and verbal commands.

enum bordermaskbits {
  BORDERMASK_TOP    = 0x1,
  BORDERMASK_RIGHT  = 0x2,
  BORDERMASK_BOTTOM = 0x4,
  BORDERMASK_LEFT   = 0x8,
};

typedef struct panelreel_options {
  // require this many rows and columns (including borders). otherwise, a
  // message will be displayed stating that a larger terminal is necessary, and
  // input will be queued. if 0, no minimum will be enforced. may not be
  // negative. note that panelreel_create() does not return error if given a
  // WINDOW smaller than these minima; it instead patiently waits for the
  // screen to get bigger.
  int min_supported_cols;
  int min_supported_rows;

  // use no more than this many rows and columns (including borders). may not be
  // less than the corresponding minimum. 0 means no maximum.
  int max_supported_cols;
  int max_supported_rows;

  // desired offsets within the surrounding WINDOW (top right bottom left) upon
  // creation / resize. a panelreel_move() operation updates these.
  int toff, roff, boff, loff;
  // is scrolling infinite (can one move down or up forever, or is an end
  // reached?). if true, 'circular' specifies how to handle the special case of
  // an incompletely-filled reel.
  bool infinitescroll;
  // is navigation circular (does moving down from the last panel move to the
  // first, and vice versa)? only meaningful when infinitescroll is true. if
  // infinitescroll is false, this must be false.
  bool circular;
  // outcurses can draw a border around the panelreel, and also around the
  // component tablets. inhibit borders by setting all valid bits in the masks.
  // partially inhibit borders by setting individual bits in the masks. the
  // appropriate attr and pair values will be used to style the borders.
  // focused and non-focused tablets can have different styles. you can instead
  // draw your own borders, or forgo borders entirely.
  unsigned bordermask; // bitfield; 1s will not be drawn. taken from bordermaskbits
  attr_t borderattr;   // attributes used for panelreel border, no color!
  int borderpair;      // extended color pair for panelreel border
  unsigned tabletmask; // bitfield; same as bordermask but for tablet borders
  attr_t tabletattr;   // attributes used for tablet borders, no color!
  int tabletpair;      // extended color pair for tablet borders
  attr_t focusedattr;  // attributes used for focused tablet borders, no color!
  int focusedpair;     // extended color pair for focused tablet borders
} panelreel_options;

struct tablet;
struct panelreel;

// Create a panelreel according to the provided specifications. Returns NULL on
// failure. w must be a valid WINDOW*, to which offsets are relative. Note that
// there might not be enough room for the specified offsets, in which case the
// panelreel will be clipped on the bottom and right. A minimum number of rows
// and columns can be enforced via popts. efd, if non-negative, is an eventfd
// that ought be written to whenever panelreel_touch() updates a tablet (this
// is useful in the case of nonblocking input).
struct panelreel* panelreel_create(WINDOW* w, const panelreel_options* popts,
                                   int efd);

// Tablet draw callback, provided a tablet (from which a PANEL and the userptr
// may be extracted), the first column that may be used, the first row that may
// be used, the first column that may not be used, the first row that may not
// be used, and a bool indicating whether output ought be clipped at the top
// (true) or bottom (false). Rows and columns are zero-indexed, and both are
// relative to the PANEL's WINDOW.
//
// Regarding clipping: it is possible that the tablet is only partially
// displayed on the screen. If so, it is either partially present on the top of
// the screen, or partially present at the bottom. In the former case, the top
// is clipped (cliptop will be true), and output ought start from the end. In
// the latter case, cliptop is false, and output ought start from the beginning.
//
// Returns the number of lines of output, which ought be less than or equal to
// maxy - begy, and non-negative (negative values might be used in the future).
typedef int (*tabletcb)(struct tablet* t, int begx, int begy,
                        int maxx, int maxy, bool cliptop);

// Add a new tablet to the provided panelreel, having the callback object
// opaque. Neither, either, or both of after and before may be specified. If
// neither is specified, the new tablet can be added anywhere on the reel. If
// one or the other is specified, the tablet will be added before or after the
// specified tablet. If both are specifid, the tablet will be added to the
// resulting location, assuming it is valid (after->next == before->prev); if
// it is not valid, or there is any other error, NULL will be returned.
struct tablet* panelreel_add(struct panelreel* pr, struct tablet* after,
                             struct tablet *before, tabletcb cb, void* opaque);

// Return the number of tablets.
int panelreel_tabletcount(const struct panelreel* pr);

// Indicate that the specified tablet has been updated in a way that would
// change its display. This will trigger some non-negative number of callbacks
// (though not in the caller's context).
int panelreel_touch(struct panelreel* pr, struct tablet* t);

// Delete the tablet specified by t from the panelreel specified by pr. Returns
// -1 if the tablet cannot be found.
int panelreel_del(struct panelreel* pr, struct tablet* t);

// Delete the active tablet. Returns -1 if there are no tablets.
int panelreel_del_focused(struct panelreel* pr);

// Move to the specified location within the containing WINDOW.
int panelreel_move(struct panelreel* pr, int x, int y);

// Redraw the panelreel in its entirety, for instance after
// clearing the screen due to external corruption, or a SIGWINCH.
int panelreel_redraw(struct panelreel* pr);

// Return the focused tablet, if any tablets are present. This is not a copy;
// be careful to use it only for the duration of a critical section.
struct tablet* panelreel_focused(struct panelreel* pr);

// Change focus to the next tablet, if one exists
struct tablet* panelreel_next(struct panelreel* pr);

// Change focus to the previous tablet, if one exists
struct tablet* panelreel_prev(struct panelreel* pr);

void* tablet_userptr(struct tablet* t);
const void* tablet_userptr_const(const struct tablet* t);
PANEL* tablet_panel(struct tablet* t);

// Destroy a panelreel allocated with panelreel_create(). Does not destroy the
// underlying WINDOW. Returns non-zero on failure.
int panelreel_destroy(struct panelreel* pr);

// Verify the panelreel's layout and appearance. Intended for unit testing.
int panelreel_validate(WINDOW* parent, struct panelreel* pr);

#define COLOR_BRIGHTWHITE 16

#ifdef __cplusplus
} // extern "C"
#endif

#endif
