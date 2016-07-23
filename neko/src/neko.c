/* neko.c */

#pragma pack(2)

#include <Pilot.h>

#include "neko.h"

/*
 * constants
 */

#define WINDOW_TOP 15

#define NEKO_WIDTH 32
#define NEKO_HEIGHT 32

#define CURSOR_WIDTH 16
#define CURSOR_HEIGHT 16

#define INTERVAL 12
#define SLEEP_INTERVAL (INTERVAL * 4)

#define MOVE_DELTA 12
#define NANAME_MOVE_DELTA 9

#define NEKO_STOP 0
#define NEKO_JARE 1
#define NEKO_KAKI 2
#define NEKO_AKUBI 3
#define NEKO_SLEEP 4
#define NEKO_AWAKE 5
#define NEKO_U_MOVE 6
#define NEKO_D_MOVE 7
#define NEKO_L_MOVE 8
#define NEKO_R_MOVE 9
#define NEKO_UL_MOVE 10
#define NEKO_UR_MOVE 11
#define NEKO_DL_MOVE 12
#define NEKO_DR_MOVE 13
#define NEKO_U_TOGI 14
#define NEKO_D_TOGI 15
#define NEKO_L_TOGI 16
#define NEKO_R_TOGI 17

#define NEKO_STOP_TIME 4
#define NEKO_JARE_TIME 10
#define NEKO_KAKI_TIME 4
#define NEKO_AKUBI_TIME 3
#define NEKO_AWAKE_TIME 3
#define NEKO_TOGI_TIME 10

#define CURSOR_OFF 0
#define CURSOR_ON 1
#define CURSOR_MOVED 2

/*
 * macros
 */

#define ABS(x) ((x) >= 0 ? (x) : -(x))

#define TRIM(x, min, max) if ((x) < (min)) { (x) = (min); } else if ((x) > (max)) { (x) = (max); }

/*
 * global variables
 */

static Int neko_state;
static Int neko_animation;
static Int neko_timer;

static Int neko_x;
static Int neko_y;

static Int cursor_state;

static Int cursor_x;
static Int cursor_y;

static Int cursor_bitmap;
static Int next_cursor;

/*
 * functions
 */

static void DrawBitmap(Int x, Int y, Int id)
{
    VoidHand h;
    VoidPtr p;

    h = DmGet1Resource('Tbmp', id);
    if (h != NULL) {
	p = MemHandleLock(h);
	WinDrawBitmap((BitmapPtr) p, x, y);
	MemHandleUnlock(h);
	DmReleaseResource(h);
    }
}

static void DrawNeko()
{
    DrawBitmap(neko_x, neko_y, NEKO_BITMAPS + neko_state * 2 + neko_animation);
    neko_animation = ! neko_animation;
    ++neko_timer;
}

static void EraseNeko()
{
    RectangleType r;
    r.topLeft.x = neko_x;
    r.topLeft.y = neko_y;
    r.extent.x = NEKO_WIDTH;
    r.extent.y = NEKO_HEIGHT;
    WinEraseRectangle(&r, 0);
}

static void DrawCursor()
{
    if (cursor_state != CURSOR_OFF) {
	DrawBitmap(cursor_x - CURSOR_WIDTH / 2, cursor_y, cursor_bitmap);
    }
}

static void EraseCursor()
{
    if (cursor_state != CURSOR_OFF) {
	RectangleType r;
	r.topLeft.x = cursor_x - CURSOR_WIDTH / 2;
	r.topLeft.y = cursor_y;
	r.extent.x = CURSOR_WIDTH;
	r.extent.y = CURSOR_HEIGHT;
	WinEraseRectangle(&r, 0);
    }
}

static void SetNekoState(Int state)
{
    neko_state = state;
    neko_animation = 0;
    neko_timer = 0;
}

static void SetCursorState(Int state, EventPtr event)
{
    cursor_state = state;
    if (event != NULL) {
	cursor_x = event->screenX;
	cursor_y = event->screenY;
    }
}

static void NekoDirection()
{
    Int dx, dy, state;

    dx = (cursor_x - NEKO_WIDTH / 2) - neko_x;
    dy = (cursor_y - NEKO_HEIGHT + CURSOR_HEIGHT / 2) - neko_y;
    if (ABS(dx) * 5 < ABS(dy) * 2) {
	state = NEKO_U_MOVE + (dy >= 0);
    } else if (ABS(dx) * 2 > ABS(dy) * 5) {
	state = NEKO_L_MOVE + (dx >= 0);
    } else {
	state = NEKO_UL_MOVE + (dy >= 0) * 2 + (dx > 0);
    }
    if (state != neko_state) {
	SetNekoState(state);
    }
}

static Boolean CalcNekoXY()
{
    Int dx, dy, max;
    Int prev_x, prev_y;

    dx = (cursor_x - NEKO_WIDTH / 2) - neko_x;
    dy = (cursor_y - NEKO_HEIGHT + CURSOR_HEIGHT / 2) - neko_y;
    if (ABS(dx) * 5 < ABS(dy) * 2) {
	max = MOVE_DELTA;
    } else if (ABS(dx) * 2 > ABS(dy) * 5) {
	max = MOVE_DELTA;
    } else {
	max = NANAME_MOVE_DELTA;
    }
    if (ABS(dx) > max || ABS(dy) > max) {
	if (ABS(dx) >= ABS(dy)) {
	    dy = dy * max / ABS(dx);
	    dx = (dx >= 0) ? max : -max;
	} else {
	    dx = dx * max / ABS(dy);
	    dy = (dy >= 0) ? max : -max;
	}
    }

    prev_x = neko_x;
    prev_y = neko_y;
    neko_x += dx;
    neko_y += dy;
    TRIM(neko_x, 0, WINDOW_WIDTH - NEKO_WIDTH)
    TRIM(neko_y, WINDOW_TOP, WINDOW_HEIGHT - NEKO_HEIGHT)

    return (neko_x != prev_x) || (neko_y != prev_y);
}

static void NekoThinkMove()
{
    EraseNeko();

    if (cursor_state == CURSOR_MOVED) {
	if (neko_state == NEKO_STOP
	 || neko_state == NEKO_JARE
	 || neko_state == NEKO_KAKI
	 || neko_state == NEKO_AKUBI
	 || neko_state == NEKO_SLEEP
	 || neko_state == NEKO_U_TOGI
	 || neko_state == NEKO_D_TOGI
	 || neko_state == NEKO_L_TOGI
	 || neko_state == NEKO_R_TOGI) {
	    SetNekoState(NEKO_AWAKE);
	    SetCursorState(CURSOR_ON, NULL);
	}
    }

    switch (neko_state) {
    case NEKO_STOP:
	if (neko_timer >= NEKO_STOP_TIME) {
	    if (cursor_x < CURSOR_WIDTH / 2) {
		SetNekoState(NEKO_L_TOGI);
	    } else if (cursor_x > WINDOW_WIDTH - CURSOR_WIDTH / 2) {
		SetNekoState(NEKO_R_TOGI);
	    } else if (cursor_y < WINDOW_TOP + CURSOR_HEIGHT / 2) {
		SetNekoState(NEKO_U_TOGI);
	    } else if (cursor_y > WINDOW_HEIGHT - CURSOR_HEIGHT / 2) {
		SetNekoState(NEKO_D_TOGI);
	    } else {
		SetNekoState(NEKO_JARE);
	    }
	}
	break;

    case NEKO_JARE:
	if (neko_timer >= NEKO_JARE_TIME) {
	    EraseCursor();
	    SetNekoState(NEKO_KAKI);
	    SetCursorState(CURSOR_OFF, NULL);
	}
	break;

    case NEKO_KAKI:
	if (neko_timer >= NEKO_KAKI_TIME) {
	    SetNekoState(NEKO_AKUBI);
	}
	break;

    case NEKO_AKUBI:
	if (neko_timer >= NEKO_AKUBI_TIME) {
	    SetNekoState(NEKO_SLEEP);
	}
	break;

    case NEKO_SLEEP:
	break;

    case NEKO_AWAKE:
	if (neko_timer >= NEKO_AWAKE_TIME) {
	    NekoDirection();
	}
	break;

    case NEKO_U_MOVE:
    case NEKO_D_MOVE:
    case NEKO_L_MOVE:
    case NEKO_R_MOVE:
    case NEKO_UL_MOVE:
    case NEKO_UR_MOVE:
    case NEKO_DL_MOVE:
    case NEKO_DR_MOVE:
	if (cursor_state == CURSOR_MOVED) {
	    NekoDirection();
	    SetCursorState(CURSOR_ON, NULL);
	}
	if (! CalcNekoXY()) {
	    SetNekoState(NEKO_STOP);
	}
	break;

    case NEKO_U_TOGI:
    case NEKO_D_TOGI:
    case NEKO_L_TOGI:
    case NEKO_R_TOGI:
	if (neko_timer >= NEKO_TOGI_TIME) {
	    EraseCursor();
	    SetNekoState(NEKO_KAKI);
	    SetCursorState(CURSOR_OFF, NULL);
	}
	break;
    }

    DrawNeko();
    DrawCursor();
}

static void InitNeko()
{
    neko_x = (WINDOW_WIDTH - NEKO_WIDTH) / 2;
    neko_y = (WINDOW_HEIGHT - NEKO_HEIGHT) / 2;
    SetNekoState(NEKO_SLEEP);
    SetCursorState(CURSOR_OFF, NULL);
    next_cursor = MOUSE_CURSOR_ID;
    DrawNeko();
}

static Boolean HandleMainFormEvent(EventPtr event)
{
    switch (event->eType) {
    case frmOpenEvent:
	FrmDrawForm(FrmGetActiveForm());
	InitNeko();
    	return true;

    case ctlSelectEvent:
    	if (event->data.ctlSelect.controlID == BIRD_CURSOR_ID
    	 || event->data.ctlSelect.controlID == MOUSE_CURSOR_ID
    	 || event->data.ctlSelect.controlID == FISH_CURSOR_ID) {
	    next_cursor = event->data.ctlSelect.controlID;
	    return true;
	}
    	if (event->data.ctlSelect.controlID == ABOUT_BUTTON_ID) {
	    FormPtr frm;
	    frm = FrmInitForm(ABOUT_FORM_ID);
	    FrmDoDialog(frm);
	    FrmDeleteForm(frm);
	    return true;
	}
	break;

    case penDownEvent:
    case penMoveEvent:
	if (event->screenY >= WINDOW_TOP) {
	    EraseCursor();
	    SetCursorState(CURSOR_MOVED, event);
	    cursor_bitmap = next_cursor;
	    NekoThinkMove();
	    return true;
	}
	break;

    case nilEvent:
	NekoThinkMove();
	return true;

    default:
	break;
    }

    return false;
}

static Boolean HandleAppEvent(EventPtr event)
{
    FormPtr form;

    if (event->eType == frmLoadEvent) {
	form = FrmInitForm(event->data.frmLoad.formID);
	FrmSetActiveForm(form);
	if (event->data.frmLoad.formID == MAIN_FORM_ID) {
	    FrmSetEventHandler(form, HandleMainFormEvent);
	}
	return true;
    }
    return false;
}

static void EventLoop()
{
    EventType event;
    Word error;

    do {
	EvtGetEvent(&event,
		neko_state == NEKO_SLEEP ? SLEEP_INTERVAL : INTERVAL);
	if (SysHandleEvent(&event) == false
	 && MenuHandleEvent(NULL, &event, &error) == false
	 && HandleAppEvent(&event) == false) {
	    FrmDispatchEvent(&event);
	}
    } while (event.eType != appStopEvent);
}

DWord PilotMain(Word cmd, Ptr cmdPBP, Word launchFlags)
{
    if (cmd == sysAppLaunchCmdNormalLaunch) {
	FrmGotoForm(MAIN_FORM_ID);
	EventLoop();
    }
    return 0;
}
