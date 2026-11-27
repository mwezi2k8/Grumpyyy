"""
Grumpyyy case - Fusion 360 script
XIAO ESP32-C3 + MPU6050 + 0.96" 128x64 OLED on a trimmed perfboard

How to run:
  Fusion 360 -> Utilities -> Scripts and Add-Ins (Shift+S) -> "+" (Create) -> Python,
  name it GrumpyyyCase, open it, replace the code with this file, save, then Run.

It makes a new design with two bodies, already laid out for printing:
  - Base : open-top tray, board sits on 4 corner posts, USB-C cutout, keyring tab
  - Lid  : top plate with OLED window and a friction-fit lip (printed face down)

All sizes are in mm. Measure your board with calipers and edit the numbers below.
Directions are as seen from the component side with the USB-C port pointing UP:
  "width"  = left to right, "height" = top (USB edge) to bottom.
"""

import adsk.core, adsk.fusion, traceback

# ======================= MEASURE AND EDIT THESE =======================
# Footprint of the whole assembly seen from above, including anything that
# sticks out past the perfboard (OLED module edge, USB-C connector tip).
BOARD_W   = 48.0   # left to right
BOARD_H   = 44.0   # top (USB edge) to bottom
PCB_T     = 1.6    # perfboard thickness

BELOW     = 5.0    # space needed UNDER the board for wires, solder blobs, pin stubs
ABOVE     = 6.5    # height of the tallest part above the perfboard top surface

# OLED window (centre measured from the LEFT edge and from the TOP/USB edge of the footprint)
WIN_X     = 16.0
WIN_Y     = 13.5
WIN_W     = 13.0   # window width  (left-right). Active area is about 11 x 22 mm
WIN_H     = 24.0   # window height (top-bottom)

# USB-C port on the XIAO (on the top wall)
USB_X     = 40.0   # centre of the USB-C port from the LEFT edge of the footprint
USB_Z     = 2.7    # centre of the USB-C port above the perfboard top surface
USB_W     = 13.0   # cutout width  (fits most cable plugs)
USB_HGT   = 7.5    # cutout height

# ======================= CASE SETTINGS =======================
CLEAR     = 0.5    # gap between board and wall on every side
WALL      = 1.8
FLOOR     = 1.5
CORNER_R  = 3.0    # outer corner radius
POST      = 3.0    # corner support post size (square)
POSTS     = True   # set False if wires run right at the corners

LID_T     = 1.6    # lid plate thickness
LIP_H     = 2.0    # lip that pushes into the base
LIP_W     = 1.2
LIP_CLR   = 0.2    # lip clearance per side (raise to 0.3 if too tight, 0.1 if loose)

TAB       = True   # keyring tab on the bottom wall
TAB_W     = 10.0
TAB_L     = 9.0    # how far the tab sticks out
TAB_T     = 3.0
RING_D    = 4.5    # keyring hole diameter

GAP_BETWEEN_PARTS = 10.0   # spacing of base and lid in the design
# ==============================================================

_mgr = None


def cm(v):
    return v / 10.0  # Fusion API works in cm


def box(x0, y0, z0, x1, y1, z1):
    xa, xb = min(x0, x1), max(x0, x1)
    ya, yb = min(y0, y1), max(y0, y1)
    za, zb = min(z0, z1), max(z0, z1)
    centre = adsk.core.Point3D.create(cm((xa + xb) / 2), cm((ya + yb) / 2), cm((za + zb) / 2))
    obb = adsk.core.OrientedBoundingBox3D.create(
        centre,
        adsk.core.Vector3D.create(1, 0, 0),
        adsk.core.Vector3D.create(0, 1, 0),
        cm(xb - xa), cm(yb - ya), cm(zb - za))
    return _mgr.createBox(obb)


def cyl_z(x, y, r, z0, z1):
    return _mgr.createCylinderOrCone(
        adsk.core.Point3D.create(cm(x), cm(y), cm(z0)), cm(r),
        adsk.core.Point3D.create(cm(x), cm(y), cm(z1)), cm(r))


def union(a, b):
    _mgr.booleanOperation(a, b, adsk.fusion.BooleanTypes.UnionBooleanType)
    return a


def cut(a, b):
    _mgr.booleanOperation(a, b, adsk.fusion.BooleanTypes.DifferenceBooleanType)
    return a


def rbox(x0, y0, z0, x1, y1, z1, r):
    """Box with rounded vertical edges."""
    r = min(r, (x1 - x0) / 2 - 0.01, (y1 - y0) / 2 - 0.01)
    if r < 0.1:
        return box(x0, y0, z0, x1, y1, z1)
    b = box(x0 + r, y0, z0, x1 - r, y1, z1)
    union(b, box(x0, y0 + r, z0, x1, y1 - r, z1))
    for cx, cy in ((x0 + r, y0 + r), (x1 - r, y0 + r), (x0 + r, y1 - r), (x1 - r, y1 - r)):
        union(b, cyl_z(cx, cy, r, z0, z1))
    return b


def build():
    # Frame: inner cavity from (0,0) to (CW,CH), floor top at z = 0, USB wall at y = CH
    CW = BOARD_W + 2 * CLEAR
    CH = BOARD_H + 2 * CLEAR
    D = BELOW + PCB_T + ABOVE                  # inner depth of the base
    R_IN = max(CORNER_R - WALL, 0.0)
    board_top = BELOW + PCB_T

    def fx(x):          # footprint x (from left) -> cavity x
        return CLEAR + x

    def fy(y_from_top):  # footprint y (from top/USB edge) -> cavity y
        return CLEAR + BOARD_H - y_from_top

    # ---------------- BASE ----------------
    base = rbox(-WALL, -WALL, -FLOOR, CW + WALL, CH + WALL, D, CORNER_R)
    cut(base, rbox(0, 0, 0, CW, CH, D + 1, R_IN))

    if POSTS:
        for px, py in ((0, 0), (CW - POST, 0), (0, CH - POST), (CW - POST, CH - POST)):
            union(base, box(px, py, -0.01, px + POST, py + POST, BELOW))

    # USB-C cutout through the top wall
    ux = fx(USB_X)
    uz = board_top + USB_Z
    cut(base, box(ux - USB_W / 2, CH - 1, uz - USB_HGT / 2,
                  ux + USB_W / 2, CH + WALL + 1, uz + USB_HGT / 2))

    # Keyring tab on the bottom wall
    if TAB:
        tcx = CW / 2
        tcy = -WALL - TAB_L + TAB_W / 2
        tab = box(tcx - TAB_W / 2, tcy, -FLOOR, tcx + TAB_W / 2, -WALL + 1.0, -FLOOR + TAB_T)
        union(tab, cyl_z(tcx, tcy, TAB_W / 2, -FLOOR, -FLOOR + TAB_T))
        union(base, tab)
        cut(base, cyl_z(tcx, tcy, RING_D / 2, -FLOOR - 1, -FLOOR + TAB_T + 1))

    # ---------------- LID (built face-down for printing) ----------------
    # Flipping the lid over mirrors left/right, so x positions are mirrored here.
    ox = CW + 2 * WALL + GAP_BETWEEN_PARTS

    def mx(x):
        return ox + (CW - x)

    lid = rbox(ox - WALL, -WALL, 0, ox + CW + WALL, CH + WALL, LID_T, CORNER_R)

    lip = rbox(ox + LIP_CLR, LIP_CLR, LID_T - 0.01,
               ox + CW - LIP_CLR, CH - LIP_CLR, LID_T + LIP_H, max(R_IN - LIP_CLR, 0))
    cut(lip, rbox(ox + LIP_CLR + LIP_W, LIP_CLR + LIP_W, LID_T - 1,
                  ox + CW - LIP_CLR - LIP_W, CH - LIP_CLR - LIP_W, LID_T + LIP_H + 1,
                  max(R_IN - LIP_CLR - LIP_W, 0)))
    # gap in the lip where the USB plug goes in
    lux = mx(fx(USB_X))
    cut(lip, box(lux - USB_W / 2 - 0.5, CH - LIP_CLR - LIP_W - 1, LID_T - 1,
                 lux + USB_W / 2 + 0.5, CH + 1, LID_T + LIP_H + 1))
    union(lid, lip)

    # OLED window
    wx = mx(fx(WIN_X))
    wy = fy(WIN_Y)
    cut(lid, rbox(wx - WIN_W / 2, wy - WIN_H / 2, -1,
                  wx + WIN_W / 2, wy + WIN_H / 2, LID_T + LIP_H + 1, 1.0))

    return base, lid


def run(context):
    global _mgr
    ui = None
    try:
        app = adsk.core.Application.get()
        ui = app.userInterface
        app.documents.add(adsk.core.DocumentTypes.FusionDesignDocumentType)
        design = adsk.fusion.Design.cast(app.activeProduct)
        design.designType = adsk.fusion.DesignTypes.DirectDesignType
        root = design.rootComponent
        _mgr = adsk.fusion.TemporaryBRepManager.get()

        base, lid = build()
        b = root.bRepBodies.add(base)
        b.name = "Base"
        l = root.bRepBodies.add(lid)
        l.name = "Lid"

        ui.messageBox(
            "Grumpyyy case created.\n\n"
            "Base: print open side up.\n"
            "Lid: print as shown (window face down on the bed).\n\n"
            "Right-click each body > Save As Mesh to export STL.")
    except:
        if ui:
            ui.messageBox("Failed:\n{}".format(traceback.format_exc()))
