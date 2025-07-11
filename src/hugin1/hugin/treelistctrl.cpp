/////////////////////////////////////////////////////////////////////////////
// Name:        treelistctrl.cpp
// Purpose:     multi column tree control implementation
// Created:     01/02/97
// Author:      Robert Roebling
// Maintainer:  Ronan Chartois (pgriddev)
// Version:     $Id: treelistctrl.cpp 3062 2012-09-23 13:48:23Z pgriddev $
// Copyright:   (c) 2004-2011 Robert Roebling, Julian Smart, Alberto Griggio,
//              Vadim Zeitlin, Otto Wyss, Ronan Chartois
// Licence:     wxWindows
/////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

#if defined(__GNUG__) && !defined(__APPLE__)
  #pragma implementation "treelistctrl.h"
#endif

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif


#include <wx/app.h>
#include <wx/treebase.h>
#include <wx/timer.h>
#include <wx/textctrl.h>
#include <wx/imaglist.h>
#include <wx/settings.h>
#include <wx/dcclient.h>
#include <wx/dcscreen.h>
#include <wx/scrolwin.h>
#include <wx/dcmemory.h>
#include <wx/renderer.h>
#include <wx/apptrait.h>
#include <wx/dcbuffer.h>
#include <wx/tooltip.h>
#include <wx/hashmap.h>
#include <wx/dynarray.h>
#include <wx/arrimpl.cpp>
#if wxCHECK_VERSION(3,1,1)
#include <wx/itemattr.h>
// wxTreeItemAttr was renamed to wxItemAttr
// instead of replacing all occurances, create this placeholder
class wxTreeItemAttr : public wxItemAttr
{};
#endif

#if defined(__WXMAC__) && defined(__WXOSX__)
#include "wx/osx/private.h"
#elif defined(__WXMAC__)
#include "wx/mac/private.h"
#endif

#include "treelistctrl.h"

#include <wx/log.h>  // only required for debugging purpose
#include <wx/msgdlg.h>  // only required for debugging purpose
#include <algorithm>


namespace wxcode {

// ---------------------------------------------------------------------------
// array types
// ---------------------------------------------------------------------------

class wxTreeListItem;
class wxTreeListItemCellAttr;

WX_DEFINE_ARRAY_PTR(wxTreeListItem *, wxArrayTreeListItems);
WX_DECLARE_OBJARRAY(wxTreeListColumnInfo, wxArrayTreeListColumnInfo);
WX_DEFINE_OBJARRAY(wxArrayTreeListColumnInfo);


WX_DECLARE_HASH_MAP( int, wxTreeListItemCellAttr *, wxIntegerHash, wxIntegerEqual, wxTreeListItemCellAttrHash );

// --------------------------------------------------------------------------
// constants
// --------------------------------------------------------------------------

static const int NO_IMAGE = -1;

static const int LINEHEIGHT = 10;
static const int LINEATROOT = 5;
static const int MARGIN = 2;
static const int MININDENT = 16;
static const int BTNWIDTH = 9;
static const int BTNHEIGHT = 9;
static const int EXTRA_WIDTH = 4;
static const int EXTRA_HEIGHT = 4;
static const int HEADER_OFFSET_X = 0;  // changed from 1 to 0 on 2009.03.10 for Windows (other OS untested)
static const int HEADER_OFFSET_Y = 1;

static const int DRAG_TIMER_TICKS = 250; // minimum drag wait time in ms
static const int FIND_TIMER_TICKS = 500; // minimum find wait time in ms
static const int RENAME_TIMER_TICKS = 250; // minimum rename wait time in ms

const wxChar* wxTreeListCtrlNameStr = _T("treelistctrl");

static wxTreeListColumnInfo wxInvalidTreeListColumnInfo;


// ---------------------------------------------------------------------------
// private classes
// ---------------------------------------------------------------------------

class  wxTreeListHeaderWindow : public wxWindow
{
protected:
    wxTreeListMainWindow *m_owner;
    const wxCursor *m_currentCursor;
    const wxCursor *m_resizeCursor;
    bool m_isDragging;

    // column being resized
    int m_column;

    // divider line position in logical (unscrolled) coords
    int m_currentX;

    // minimal position beyond which the divider line can't be dragged in
    // logical coords
    int m_minX;

    wxArrayTreeListColumnInfo m_columns;

    // total width of the columns
    int m_total_col_width;

    // which col header is currently highlighted with mouse-over
    int m_hotTrackCol;
    int XToCol(int x);
    void RefreshColLabel(int col);

public:
    wxTreeListHeaderWindow();

    wxTreeListHeaderWindow( wxWindow *win,
                            wxWindowID id,
                            wxTreeListMainWindow *owner,
                            const wxPoint &pos = wxDefaultPosition,
                            const wxSize &size = wxDefaultSize,
                            long style = 0,
                            const wxString &name = _T("wxtreelistctrlcolumntitles") );

    virtual ~wxTreeListHeaderWindow();

    void DoDrawRect( wxDC *dc, int x, int y, int w, int h );
    void DrawCurrent();
    void AdjustDC(wxDC& dc);

    void OnPaint( wxPaintEvent &event );
    void OnEraseBackground(wxEraseEvent& WXUNUSED(event)) { ;; } // reduce flicker
    void OnMouse( wxMouseEvent &event );
    void OnSetFocus( wxFocusEvent &event );

    // total width of all columns
    int GetWidth() const { return m_total_col_width; }

    // column manipulation
    int GetColumnCount() const { return (int)m_columns.GetCount(); }

    void AddColumn (const wxTreeListColumnInfo& colInfo);

    void InsertColumn (int before, const wxTreeListColumnInfo& colInfo);

    void RemoveColumn (int column);

    // column information manipulation
    const wxTreeListColumnInfo& GetColumn (int column) const{
        wxCHECK_MSG ((column >= 0) && (column < GetColumnCount()),
                     wxInvalidTreeListColumnInfo, _T("Invalid column"));
        return m_columns[column];
    }
    wxTreeListColumnInfo& GetColumn (int column) {
        wxCHECK_MSG ((column >= 0) && (column < GetColumnCount()),
                     wxInvalidTreeListColumnInfo, _T("Invalid column"));
        return m_columns[column];
    }
    void SetColumn (int column, const wxTreeListColumnInfo& info);

    wxString GetColumnText (int column) const {
        wxCHECK_MSG ((column >= 0) && (column < GetColumnCount()),
                     wxEmptyString, _T("Invalid column"));
        return m_columns[column].GetText();
    }
    void SetColumnText (int column, const wxString& text) {
        wxCHECK_RET ((column >= 0) && (column < GetColumnCount()),
                     _T("Invalid column"));
        m_columns[column].SetText (text);
    }

    int GetColumnAlignment (int column) const {
        wxCHECK_MSG ((column >= 0) && (column < GetColumnCount()),
                     wxALIGN_LEFT, _T("Invalid column"));
        return m_columns[column].GetAlignment();
    }
    void SetColumnAlignment (int column, int flag) {
        wxCHECK_RET ((column >= 0) && (column < GetColumnCount()),
                     _T("Invalid column"));
        m_columns[column].SetAlignment (flag);
    }

    int GetColumnWidth (int column) const {
        wxCHECK_MSG ((column >= 0) && (column < GetColumnCount()),
                     -1, _T("Invalid column"));
        return m_columns[column].GetWidth();
    }
    void SetColumnWidth (int column, int width);

    bool IsColumnEditable (int column) const {
        wxCHECK_MSG ((column >= 0) && (column < GetColumnCount()),
                     false, _T("Invalid column"));
        return m_columns[column].IsEditable();
    }

    bool IsColumnShown (int column) const {
        wxCHECK_MSG ((column >= 0) && (column < GetColumnCount()),
                     true, _T("Invalid column"));
        return m_columns[column].IsShown();
    }

    // needs refresh
    bool m_dirty;

private:
    // common part of all ctors
    void Init();

    void SendListEvent(wxEventType type, wxPoint pos);

    DECLARE_DYNAMIC_CLASS(wxTreeListHeaderWindow)
    DECLARE_EVENT_TABLE()
};


//-----------------------------------------------------------------------------

class wxEditTextCtrl;


// this is the "true" control
class  wxTreeListMainWindow: public wxScrolledWindow
{
friend class wxTreeListItem;
friend class wxTreeListRenameTimer;
friend class wxEditTextCtrl;

public:
    // creation
    // --------
    wxTreeListMainWindow() { Init(); }

    wxTreeListMainWindow (wxTreeListCtrl *parent, wxWindowID id = -1,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = wxTR_DEFAULT_STYLE,
               const wxValidator &validator = wxDefaultValidator,
               const wxString& name = _T("wxtreelistmainwindow"))
    {
        Init();
        Create (parent, id, pos, size, style, validator, name);
    }

    virtual ~wxTreeListMainWindow();

    bool Create(wxTreeListCtrl *parent, wxWindowID id = -1,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxTR_DEFAULT_STYLE,
                const wxValidator &validator = wxDefaultValidator,
                const wxString& name = _T("wxtreelistctrl"));

    // accessors
    // ---------

    // return true if this is a virtual list control
    bool IsVirtual() const { return HasFlag(wxTR_VIRTUAL); }

    // get the total number of items in the control
    size_t GetCount() const;

    // indent is the number of pixels the children are indented relative to
    // the parents position. SetIndent() also redraws the control
    // immediately.
    unsigned int GetIndent() const { return m_indent; }
    void SetIndent(unsigned int indent);

    // see wxTreeListCtrl for the meaning
    unsigned int GetLineSpacing() const { return m_linespacing; }
    void SetLineSpacing(unsigned int spacing);

    // image list: these functions allow to associate an image list with
    // the control and retrieve it. Note that when assigned with
    // SetImageList, the control does _not_ delete
    // the associated image list when it's deleted in order to allow image
    // lists to be shared between different controls. If you use
    // AssignImageList, the control _does_ delete the image list.

    // The normal image list is for the icons which correspond to the
    // normal tree item state (whether it is selected or not).
    // Additionally, the application might choose to show a state icon
    // which corresponds to an app-defined item state (for example,
    // checked/unchecked) which are taken from the state image list.
    wxImageList *GetImageList() const { return m_imageListNormal; }
    wxImageList *GetStateImageList() const { return m_imageListState; }
    wxImageList *GetButtonsImageList() const { return m_imageListButtons; }

    void SetImageList(wxImageList *imageList);
    void SetStateImageList(wxImageList *imageList);
    void SetButtonsImageList(wxImageList *imageList);
    void AssignImageList(wxImageList *imageList);
    void AssignStateImageList(wxImageList *imageList);
    void AssignButtonsImageList(wxImageList *imageList);

    void SetToolTip(const wxString& tip);
    void SetToolTip(wxToolTip *tip);
    void SetItemToolTip(const wxTreeItemId& item, const wxString &tip);

    // Functions to work with tree ctrl items.



    // accessors (most props have a default at row/item level *and* a default at cell level)
    // ---------

    wxString GetItemText (const wxTreeItemId& item, int column) const;
    wxString GetItemText (wxTreeItemData* item, int column) const;

    // ItemImage is special: main col has multiple images
    int GetItemImage (const wxTreeItemId& item,             wxTreeItemIcon which = wxTreeItemIcon_Normal) const  { return GetItemImage (item, GetMainColumn(), which); }
    int GetItemImage (const wxTreeItemId& item, int column, wxTreeItemIcon which = wxTreeItemIcon_Normal) const;

    // ItemData is special, there is a separate default at row/item level
    wxTreeItemData *GetItemData(const wxTreeItemId& item) const;
    wxTreeItemData *GetItemData(const wxTreeItemId& item, int column) const;

    bool GetItemBold(const wxTreeItemId& item)             const;
    bool GetItemBold(const wxTreeItemId& item, int column) const;

    wxColour GetItemTextColour(const wxTreeItemId& item)             const;
    wxColour GetItemTextColour(const wxTreeItemId& item, int column) const;

    wxColour GetItemBackgroundColour(const wxTreeItemId& item)             const;
    wxColour GetItemBackgroundColour(const wxTreeItemId& item, int column) const;

    wxFont GetItemFont(const wxTreeItemId& item) const;
    wxFont GetItemFont(const wxTreeItemId& item, int column) const;



    // modifiers (most properties have a default at row/item level *and* a default at cell level)
    // ---------

    // force appearance of [+] button near the item. This is useful to
    // allow the user to expand the items which don't have any children now
    // - but instead add them only when needed, thus minimizing memory
    // usage and loading time.
    void SetItemHasChildren(const wxTreeItemId& item, bool has = true);

    // set item's label
    void SetItemText (const wxTreeItemId& item, int column, const wxString& text);

    // get one of the images associated with the item (normal by default)
    void SetItemImage (const wxTreeItemId& item,             int image, wxTreeItemIcon which = wxTreeItemIcon_Normal) { SetItemImage (item, GetMainColumn(), image, which); }
    void SetItemImage (const wxTreeItemId& item, int column, int image, wxTreeItemIcon which = wxTreeItemIcon_Normal);

    // associate some data with the item
    void SetItemData(const wxTreeItemId& item,             wxTreeItemData *data);
    void SetItemData(const wxTreeItemId& item, int column, wxTreeItemData *data);

    // the item will be shown in bold
    void SetItemBold(const wxTreeItemId& item,             bool bold = true);
    void SetItemBold(const wxTreeItemId& item, int column, bool bold = true);

    // set the item's text colour
    void SetItemTextColour(const wxTreeItemId& item,             const wxColour& colour);
    void SetItemTextColour(const wxTreeItemId& item, int column, const wxColour& colour);

    // set the item's background colour
    void SetItemBackgroundColour(const wxTreeItemId& item,             const wxColour& colour);
    void SetItemBackgroundColour(const wxTreeItemId& item, int column, const wxColour& colour);

    // set the item's font (should be of the same height for all items)
    void SetItemFont(const wxTreeItemId& item,             const wxFont& font);
    void SetItemFont(const wxTreeItemId& item, int column, const wxFont& font);



    // item status inquiries
    // ---------------------

    // is the item visible (it might be outside the view or not expanded)?
    bool IsVisible(const wxTreeItemId& item, bool fullRow, bool within = true) const;
    // does the item has any children?
    bool HasChildren(const wxTreeItemId& item) const;
    // is the item expanded (only makes sense if HasChildren())?
    bool IsExpanded(const wxTreeItemId& item) const;
    // is this item currently selected (the same as has focus)?
    bool IsSelected(const wxTreeItemId& item) const;
    // is item text in bold font?
    bool IsBold(const wxTreeItemId& item)             const;
    bool IsBold(const wxTreeItemId& item, int column) const;



    // set the window font
    virtual bool SetFont( const wxFont &font );

    // set the styles.  No need to specify a GetWindowStyle here since
    // the base wxWindow member function will do it for us
    void SetWindowStyle(const long styles);

    // number of children
    // ------------------

    // if 'recursively' is false, only immediate children count, otherwise
    // the returned number is the number of all items in this branch
    size_t GetChildrenCount(const wxTreeItemId& item, bool recursively = true);

    // navigation
    // ----------

    // wxTreeItemId.IsOk() will return false if there is no such item

    // get the root tree item
    wxTreeItemId GetRootItem() const { return m_rootItem; }  // implict cast from wxTreeListItem *

    // get the item currently selected, only if a single item is selected
    wxTreeItemId GetSelection() const { return m_selectItem; }

    // get all the items currently selected, return count of items
    size_t GetSelections(wxArrayTreeItemIds&) const;

    // get the parent of this item (may return NULL if root)
    wxTreeItemId GetItemParent(const wxTreeItemId& item) const;

    // for this enumeration function you must pass in a "cookie" parameter
    // which is opaque for the application but is necessary for the library
    // to make these functions reentrant (i.e. allow more than one
    // enumeration on one and the same object simultaneously). Of course,
    // the "cookie" passed to GetFirstChild() and GetNextChild() should be
    // the same!

    // get child of this item
    wxTreeItemId GetFirstChild(const wxTreeItemId& item, wxTreeItemIdValue& cookie) const;
    wxTreeItemId GetNextChild(const wxTreeItemId& item, wxTreeItemIdValue& cookie) const;
    wxTreeItemId GetPrevChild(const wxTreeItemId& item, wxTreeItemIdValue& cookie) const;
    wxTreeItemId GetLastChild(const wxTreeItemId& item, wxTreeItemIdValue& cookie) const;

    // get sibling of this item
    wxTreeItemId GetNextSibling(const wxTreeItemId& item) const;
    wxTreeItemId GetPrevSibling(const wxTreeItemId& item) const;

    // get item in the full tree (currently only for internal use)
    wxTreeItemId GetNext(const wxTreeItemId& item, bool fulltree = true) const;
    wxTreeItemId GetPrev(const wxTreeItemId& item, bool fulltree = true) const;

    // get expanded item, see IsExpanded()
    wxTreeItemId GetFirstExpandedItem() const;
    wxTreeItemId GetNextExpanded(const wxTreeItemId& item) const;
    wxTreeItemId GetPrevExpanded(const wxTreeItemId& item) const;

    // get visible item, see IsVisible()
    wxTreeItemId GetFirstVisible(                          bool fullRow, bool within) const;
    wxTreeItemId GetNextVisible (const wxTreeItemId& item, bool fullRow, bool within) const;
    wxTreeItemId GetPrevVisible (const wxTreeItemId& item, bool fullRow, bool within) const;
    wxTreeItemId GetLastVisible (                          bool fullRow, bool within) const;

    // operations
    // ----------

    // add the root node to the tree
    wxTreeItemId AddRoot (const wxString& text,
                          int image = -1, int selectedImage = -1,
                          wxTreeItemData *data = NULL);

    // insert a new item in as the first child of the parent
    wxTreeItemId PrependItem(const wxTreeItemId& parent,
                             const wxString& text,
                             int image = -1, int selectedImage = -1,
                             wxTreeItemData *data = NULL);

    // insert a new item after a given one
    wxTreeItemId InsertItem(const wxTreeItemId& parent,
                            const wxTreeItemId& idPrevious,
                            const wxString& text,
                            int image = -1, int selectedImage = -1,
                            wxTreeItemData *data = NULL);

    // insert a new item before the one with the given index
    wxTreeItemId InsertItem(const wxTreeItemId& parent,
                            size_t index,
                            const wxString& text,
                            int image = -1, int selectedImage = -1,
                            wxTreeItemData *data = NULL);

    // insert a new item in as the last child of the parent
    wxTreeItemId AppendItem(const wxTreeItemId& parent,
                            const wxString& text,
                            int image = -1, int selectedImage = -1,
                            wxTreeItemData *data = NULL);

    // delete this item and associated data if any
    void Delete(const wxTreeItemId& item);
    // delete all children (but don't delete the item itself)
    // NB: this won't send wxEVT_COMMAND_TREE_ITEM_DELETED events
    void DeleteChildren(const wxTreeItemId& item);
    // delete the root and all its children from the tree
    // NB: this won't send wxEVT_COMMAND_TREE_ITEM_DELETED events
    void DeleteRoot();

    void SetItemParent(const wxTreeItemId& parent, const wxTreeItemId& item);

    // expand this item
    void Expand(const wxTreeItemId& item);
    // expand this item and all subitems recursively
    void ExpandAll(const wxTreeItemId& item);
    // collapse the item without removing its children
    void Collapse(const wxTreeItemId& item);
    // collapse the item and remove all children
    void CollapseAndReset(const wxTreeItemId& item);
    // toggles the current state
    void Toggle(const wxTreeItemId& item);

    // set cursor item (indicated by black rectangle)
    void SetCurrentItem(const wxTreeItemId& item);

    // remove the selection from currently selected item (if any)
    void Unselect();
    void UnselectAll();
    // select this item
    bool SelectItem(const wxTreeItemId& item, const wxTreeItemId& prev = (wxTreeItemId*)NULL,
                    bool unselect_others = true);
    void SelectAll();
    // make sure this item is visible (expanding the parent item and/or
    // scrolling to this item if necessary)
    void EnsureVisible(const wxTreeItemId& item);
    // scroll to this item (but don't expand its parent)
    void ScrollTo(const wxTreeItemId& item);
    void AdjustMyScrollbars();

    // The first function is more portable (because easier to implement
    // on other platforms), but the second one returns some extra info.
    wxTreeItemId HitTest (const wxPoint& point)
        { int flags; int column; return HitTest (point, flags, column); }
    wxTreeItemId HitTest (const wxPoint& point, int& flags)
        { int column; return HitTest (point, flags, column); }
    wxTreeItemId HitTest (const wxPoint& point, int& flags, int& column);


    // get the bounding rectangle of the item (or of its label only)
    bool GetBoundingRect(const wxTreeItemId& item,
                         wxRect& rect,
                         bool textOnly = false) const;

    // Start editing the item label: this (temporarily) replaces the item
    // with a one line edit control. The item will be selected if it hadn't
    // been before.
    void EditLabel (const wxTreeItemId& item, int column);
    void EndEdit(bool isCancelled);

    // sorting
    // this function is called to compare 2 items and should return -1, 0
    // or +1 if the first item is less than, equal to or greater than the
    // second one. The base class version performs alphabetic comparaison
    // of item labels (GetText)
    virtual int OnCompareItems(const wxTreeItemId& item1,
                               const wxTreeItemId& item2);
    // sort the children of this item using OnCompareItems
    //
    // NB: this function is not reentrant and not MT-safe (TODO)!
    void SortChildren(const wxTreeItemId& item, int column, bool reverseOrder);

    // searching
    bool MatchItemText (const wxString &itemText, const wxString &pattern, int mode);
    wxTreeItemId FindItem (const wxTreeItemId& item, int column, const wxString& str, int mode = 0);

    // implementation only from now on

    // overridden base class virtuals
    virtual bool SetBackgroundColour(const wxColour& colour);
    virtual bool SetForegroundColour(const wxColour& colour);

    // drop over item
    void SetDragItem (const wxTreeItemId& item = (wxTreeItemId*)NULL);

    // callbacks
    void OnPaint( wxPaintEvent &event );
    void OnEraseBackground(wxEraseEvent& WXUNUSED(event)) { ;; } // to reduce flicker
    void OnSetFocus( wxFocusEvent &event );
    void OnKillFocus( wxFocusEvent &event );
    void OnChar( wxKeyEvent &event );
    void OnMouse( wxMouseEvent &event );
    void OnIdle( wxIdleEvent &event );
    void OnScroll(wxScrollWinEvent& event);
    void OnCaptureLost(wxMouseCaptureLostEvent & WXUNUSED(event)) { ;; }

    // implementation helpers
    int GetColumnCount() const
    { return m_owner->GetHeaderWindow()->GetColumnCount(); }

    void SetMainColumn (int column)
    { if ((column >= 0) && (column < GetColumnCount())) m_main_column = column; }

    int GetMainColumn() const { return m_main_column; }
    int GetCurrentColumn() const { return m_curColumn >= 0 ? m_curColumn : m_main_column; }

    int GetBestColumnWidth (int column, wxTreeItemId parent = wxTreeItemId());
    int GetItemWidth (int column, wxTreeListItem *item);

    void SetFocus();

protected:
    wxTreeListCtrl* m_owner;

    wxFont               m_normalFont;
    wxFont               m_boldFont;

    wxTreeListItem       *m_rootItem; // root item
    wxTreeListItem       *m_curItem; // current item, either selected or marked
    wxTreeListItem       *m_shiftItem; // item, where the shift key was pressed
    wxTreeListItem       *m_selectItem; // current selected item, not with wxTR_MULTIPLE

    int                  m_main_column;
    int                  m_curColumn;
    int                  m_sortColumn;
    bool                 m_ReverseSortOrder;

    int                  m_btnWidth, m_btnWidth2;
    int                  m_btnHeight, m_btnHeight2;
    int                  m_imgWidth, m_imgWidth2;
    int                  m_imgHeight, m_imgHeight2;
    unsigned short       m_indent;
    int                  m_lineHeight;
    unsigned short       m_linespacing;
    wxPen                m_dottedPen;
    wxBrush             *m_hilightBrush,
                        *m_hilightUnfocusedBrush;
    bool                 m_hasFocus;
public:
    bool                 m_dirty;
protected:
    bool                 m_ownsImageListNormal,
                         m_ownsImageListState,
                         m_ownsImageListButtons;
    bool                 m_lastOnSame;  // last click on the same item as prev
    bool                 m_left_down_selection;

    wxImageList         *m_imageListNormal,
                        *m_imageListState,
                        *m_imageListButtons;

    bool                 m_isDragStarted;  // set at the very beginning of dragging
    bool                 m_isDragging; // set once a drag begin event was fired
    wxPoint              m_dragStartPos;  // set whenever m_isDragStarted is set to true
    wxTreeListItem      *m_dragItem;
    int                  m_dragCol;

    wxTreeListItem       *m_editItem; // item, which is currently edited
    wxTimer             *m_editTimer;
    bool                 m_editAccept;  // currently unused, OnRenameAccept() argument makes it redundant
    wxString             m_editRes;
    int                  m_editCol;
    wxEditTextCtrl      *m_editControl;

    // char navigation
    wxTimer             *m_findTimer;
    wxString             m_findStr;

    bool                 m_isItemToolTip;  // true if individual item tooltips were set (disable global tooltip)
    wxString             m_toolTip;  // global tooltip
    wxTreeListItem      *m_toolTipItem;  // item whose tip is currently shown (NULL==global, -1==not displayed)

    // the common part of all ctors
    void Init();

    // misc helpers
    wxTreeItemId DoInsertItem(const wxTreeItemId& parent,
                              size_t previous,
                              const wxString& text,
                              int image, int selectedImage,
                              wxTreeItemData *data);
    void DoDeleteItem (wxTreeListItem *item);
    void SetCurrentItem(wxTreeListItem *item);
    bool HasButtons(void) const
        { return (m_imageListButtons) || HasFlag (wxTR_TWIST_BUTTONS|wxTR_HAS_BUTTONS); }

    void CalculateLineHeight();
    int  GetLineHeight(wxTreeListItem *item) const;
    void PaintLevel( wxTreeListItem *item, wxDC& dc, int level, int &y,
                     int x_maincol);
    void PaintItem( wxTreeListItem *item, wxDC& dc);

    void CalculateLevel( wxTreeListItem *item, wxDC &dc, int level, int &y,
                         int x_maincol);
    void CalculatePositions();
    void CalculateSize( wxTreeListItem *item, wxDC &dc );

    void RefreshSubtree (wxTreeListItem *item);
    void RefreshLine (wxTreeListItem *item);
    // redraw all selected items
    void RefreshSelected();
    // RefreshSelected() recursive helper
    void RefreshSelectedUnder (wxTreeListItem *item);

    void OnRenameTimer();
    void OnRenameAccept(bool isCancelled);

    void FillArray(wxTreeListItem*, wxArrayTreeItemIds&) const;
    bool TagAllChildrenUntilLast (wxTreeListItem *crt_item, wxTreeListItem *last_item);
    bool TagNextChildren (wxTreeListItem *crt_item, wxTreeListItem *last_item);
    void UnselectAllChildren (wxTreeListItem *item );
    bool SendEvent(wxEventType event_type, wxTreeListItem *item = NULL, wxTreeEvent *event = NULL);  // returns true if processed

#if wxCHECK_VERSION(3,1,3)
    void OnDpiChanged(wxDPIChangedEvent& e);
#endif

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxTreeListMainWindow)
};


//-----------------------------------------------------------------------------

// timer used for enabling in-place edit
class  wxTreeListRenameTimer: public wxTimer
{
public:
    wxTreeListRenameTimer( wxTreeListMainWindow *owner );

    void Notify();

private:
    wxTreeListMainWindow   *m_owner;
};


//-----------------------------------------------------------------------------

// control used for in-place edit
class  wxEditTextCtrl: public wxTextCtrl
{
public:
    wxEditTextCtrl (wxWindow *parent,
                    const wxWindowID id,
                    bool *accept,
                    wxString *res,
                    wxTreeListMainWindow *owner,
                    const wxString &value = wxEmptyString,
                    const wxPoint &pos = wxDefaultPosition,
                    const wxSize &size = wxDefaultSize,
                    long style = 0,
                    const wxValidator& validator = wxDefaultValidator,
                    const wxString &name = wxTextCtrlNameStr );
    ~wxEditTextCtrl();

    virtual bool Destroy();  // wxWindow override
    void EndEdit(bool isCancelled);
    void SetOwner(wxTreeListMainWindow *owner) { m_owner = owner; }

    void OnChar( wxKeyEvent &event );
    void OnKeyUp( wxKeyEvent &event );
    void OnKillFocus( wxFocusEvent &event );


private:
    wxTreeListMainWindow  *m_owner;
    bool               *m_accept;
    wxString           *m_res;
    wxString            m_startValue;
    bool                m_finished;  // true==deleting, don't process events anymore

    DECLARE_EVENT_TABLE()
};


//-----------------------------------------------------------------------------

// list of per-column attributes for an item (wxTreeListItem)
// since there can be very many of these, we save size by chosing
// the smallest representation for the elements and by ordering
// the members to avoid padding.
class  wxTreeListItemCellAttr
{
public:
    wxTreeListItemCellAttr() {
        m_attr = NULL;
        m_data = NULL;
        m_isBold = 0;
        m_isBoldSet = 0;
        m_ownsAttr = 0;
        m_image = NO_IMAGE;
    };
    ~wxTreeListItemCellAttr() {
        if (m_ownsAttr) delete m_attr;
    };

    // generic attribute from wxWidgets lib
    wxTreeItemAttr      *m_attr;

    // other attributes
    wxTreeItemData      *m_data;        // user-provided data
    short                m_image;       // images for the various columns (!= main)
    int                  m_isBold :1;   // render the label in bold font
    int                  m_isBoldSet :1;   // was 'm_isBold' set ?
    int                  m_ownsAttr :1; // delete attribute when done
};


//-----------------------------------------------------------------------------

// a tree item (NOTE: this class is storage only, does not generate events)
class  wxTreeListItem
{
public:
    // ctors & dtor
    // ------------
    wxTreeListItem() { m_toolTip = NULL; }
    wxTreeListItem( wxTreeListMainWindow *owner,
                    wxTreeListItem *parent,
                    const wxArrayString& text,
                    int image,
                    int selImage,
                    wxTreeItemData *data );

    ~wxTreeListItem();


    // accessors (most properties have a default at row/item level)
    // ---------
    wxArrayTreeListItems& GetChildren() { return m_children; }

//    const wxString GetText (          ) const { return GetText(m_owner->GetMainColumn());  }
    const wxString GetText (int column) const
    {
        if ( IsVirtual() )   return m_owner->GetItemText( m_props_row.m_data, column );
        if (column < (signed)m_text.GetCount()) return m_text[column];
        return wxEmptyString;
    };

    int GetImage (            wxTreeItemIcon which = wxTreeItemIcon_Normal) const { return m_images[which]; };
    int GetImage (int column, wxTreeItemIcon which = wxTreeItemIcon_Normal) const
    {
        // main column is special, more images available
        if(column == m_owner->GetMainColumn()) return m_images[which];

        // other columns ignore the 'which' parameter
        wxTreeListItemCellAttrHash::const_iterator entry = m_props_cell.find( column );
        if (entry == m_props_cell.end()) return NO_IMAGE;
        return entry->second->m_image;
    };

    // data is special: it has a default value at row/item level
    wxTreeItemData *GetData()           const { return m_props_row.m_data; };
    wxTreeItemData *GetData(int column) const {
        wxTreeListItemCellAttrHash::const_iterator entry = m_props_cell.find( column );
        if (entry == m_props_cell.end()) return NULL;
        return entry->second->m_data;
    };

    const wxString * GetToolTip() const  {  return m_toolTip;  };

    // returns the current image for the item (depending on its
    // selected/expanded/whatever state)
    int GetCurrentImage() const;


    // modifiers (most properties have a default at row/item level)
    // ---------
    void SetHasPlus(bool has = true) { m_hasPlus = has; };

    void SetText (int column, const wxString& text)
    {
        if (column < (int)m_text.GetCount()) {
            m_text[column] = text;
        } else if (column < m_owner->GetColumnCount()) {
            int howmany = m_owner->GetColumnCount();
            for (int i = (int)m_text.GetCount(); i < howmany; ++i)
            {
                m_text.Add(wxEmptyString);
                m_text_x.Add(0);
            };
            m_text[column] = text;
        }
    };
    void SetImage (            int image, wxTreeItemIcon which) { m_images[which] = image; };
    void SetImage (int column, int image, wxTreeItemIcon which)
    {
        // main column is special, more images available
        if (column == m_owner->GetMainColumn()) m_images[which] = image;
        // other columns ignore the 'which' parameter
        else {
            wxTreeListItemCellAttrHash::const_iterator entry = m_props_cell.find( column );
            if (entry == m_props_cell.end()) {
                m_props_cell[column] = new wxTreeListItemCellAttr();
                m_props_cell[column]->m_image = image;
            } else {
                entry->second->m_image = image;
            }
        }
    };

    // data is special: it has a default value at row/item level
    void SetData(            wxTreeItemData *data) { m_props_row.m_data = data; };
    void SetData(int column, wxTreeItemData *data)
    {
        wxTreeListItemCellAttrHash::const_iterator entry = m_props_cell.find( column );
        if (entry == m_props_cell.end()) {
            m_props_cell[column] = new wxTreeListItemCellAttr();
            m_props_cell[column]->m_data = data;
        } else {
            entry->second->m_data = data;
        }
    }

    void SetBold(            bool bold) { m_props_row.m_isBold = bold; }
    void SetBold(int column, bool bold)
    {
        wxTreeListItemCellAttrHash::const_iterator entry = m_props_cell.find( column );
        if (entry == m_props_cell.end()) {
            m_props_cell[column] = new wxTreeListItemCellAttr();
            m_props_cell[column]->m_isBold = bold;
            m_props_cell[column]->m_isBoldSet = 1;
        } else {
            entry->second->m_isBold = bold;
            entry->second->m_isBoldSet = 1;
        }
    }


    void SetToolTip(const wxString &tip) {
        if (m_toolTip)  { delete m_toolTip; m_toolTip = NULL; }
        if (tip.length() > 0) { m_toolTip = new wxString(tip); }
    };


    // status inquiries
    // ----------------
    bool HasChildren() const        { return !m_children.IsEmpty(); }
    bool IsSelected()  const        { return m_hasHilight != 0; }
    bool IsExpanded()  const        { return !m_isCollapsed; }
    bool HasPlus()     const        { return m_hasPlus || HasChildren(); }
    bool IsBold()      const        { return m_props_row.m_isBold != 0; }
    bool IsBold(int column) const
    {
        wxTreeListItemCellAttrHash::const_iterator entry = m_props_cell.find( column );
        if (entry == m_props_cell.end() || ! entry->second->m_isBoldSet) return IsBold();
        return (entry->second->m_isBold != 0);
    }
    bool IsVirtual()   const        { return m_owner->IsVirtual(); }



    int GetX() const { return m_x; }
    int GetY() const { return m_y; }

    void SetX (int x) { m_x = x; }
    void SetY (int y) { m_y = y; }

    int  GetHeight() const { return m_height; }
    int  GetWidth()  const { return m_width; }

    void SetHeight (int height) { m_height = height; }
    void SetWidth (int width) { m_width = width; }

    int GetTextX(int column) const
    { 
        if (column >=0 && column < (signed)m_text_x.GetCount())
        {
            return m_text_x[column];
        };
        return 0;
    }
    void SetTextX (int column, int text_x) { if (column >=0 && column < (signed)m_text_x.GetCount()) m_text_x[column] = text_x; }

    wxTreeListItem *GetItemParent() const { return m_parent; }
    void SetItemParent(wxTreeListItem *parent) { m_parent = parent; }

    // get count of all children (and grand children if 'recursively')
    size_t GetChildrenCount(bool recursively = true) const;

    void GetSize( int &x, int &y, const wxTreeListMainWindow* );

    // return the item at given position (or NULL if no item), onButton is
    // true if the point belongs to the item's button, otherwise it lies
    // on the button's label
    wxTreeListItem *HitTest (const wxPoint& point,
                             const wxTreeListMainWindow *,
                             int &flags, int& column, int level);


    // operations
    // ----------
    // deletes all children
    void DeleteChildren();

    void Insert(wxTreeListItem *child, size_t index)
    { m_children.Insert(child, index); }

    void Expand() { m_isCollapsed = false; }
    void Collapse() { m_isCollapsed = true; }

    void SetHilight( bool set = true ) { m_hasHilight = set; }


    // attributes
    // ----------

    // get them - may be NULL (used to read attributes)
    // NOTE: fall back on default at row/item level is not defined for cell
    wxTreeItemAttr *GetAttributes(int column) const
    {
        wxTreeListItemCellAttrHash::const_iterator entry = m_props_cell.find( column );
        if (entry == m_props_cell.end()) return GetAttributes();
        return entry->second->m_attr;
    }
    wxTreeItemAttr *GetAttributes() const { return m_props_row.m_attr; }

    // get them ensuring that the pointer is not NULL (used to write attributes)
    wxTreeItemAttr& Attr(int column) {
        wxTreeListItemCellAttrHash::const_iterator entry = m_props_cell.find( column );
        if (entry == m_props_cell.end()) {
            m_props_cell[column] = new wxTreeListItemCellAttr();
            m_props_cell[column]->m_attr = new wxTreeItemAttr;
            m_props_cell[column]->m_ownsAttr = 1;
            return *(m_props_cell[column]->m_attr);
        } else {
            return *(entry->second->m_attr);
        }
    }
    wxTreeItemAttr& Attr()
    {
        if ( !m_props_row.m_attr )
        {
            m_props_row.m_attr = new wxTreeItemAttr;
            m_props_row.m_ownsAttr = 1;
        }
        return *m_props_row.m_attr;
    }
/* ----- unused -----
    // set them
    void SetAttributes(wxTreeItemAttr *attr)
    {
        if ( m_props_row.m_ownsAttr ) delete m_props_row.m_attr;
        m_props_row.m_attr = attr;
        m_props_row.m_ownsAttr = 0;
    }
    // set them and delete when done
    void AssignAttributes(wxTreeItemAttr *attr)
    {
        SetAttributes(attr);
        m_props_row.m_ownsAttr = 1;
    }
*/

private:
    wxTreeListMainWindow       *m_owner;        // control the item belongs to

    wxArrayTreeListItems        m_children;     // list of children
    wxTreeListItem             *m_parent;       // parent of this item

    // main column item positions
    wxCoord                     m_x;            // (virtual) offset from left (vertical line)
    wxCoord                     m_y;            // (virtual) offset from top
    short                       m_width;        // width of this item
    unsigned char               m_height;       // height of this item

    // for the normal, selected, expanded and expanded+selected states
    short                       m_images[wxTreeItemIcon_Max];
    // currently there is no tooltip at cell level
    wxString                   *m_toolTip;

    // use bitfields to save size
    int                         m_isCollapsed :1;
    int                         m_hasHilight  :1; // same as focused
    int                         m_hasPlus     :1; // used for item which doesn't have
                                                    // children but has a [+] button

    // here are all the properties which can be set per column
    wxArrayString               m_text;        // labels to be rendered for item
    wxArrayLong                 m_text_x;
    wxTreeListItemCellAttr      m_props_row;   // default at row/item level for: data, attr
    wxTreeListItemCellAttrHash  m_props_cell;
};


// ===========================================================================
// implementation
// ===========================================================================

// ---------------------------------------------------------------------------
// wxTreeListRenameTimer (internal)
// ---------------------------------------------------------------------------

wxTreeListRenameTimer::wxTreeListRenameTimer( wxTreeListMainWindow *owner )
{
    m_owner = owner;
}

void wxTreeListRenameTimer::Notify()
{
    m_owner->OnRenameTimer();
}

//-----------------------------------------------------------------------------
// wxEditTextCtrl (internal)
//-----------------------------------------------------------------------------

BEGIN_EVENT_TABLE (wxEditTextCtrl,wxTextCtrl)
    EVT_CHAR           (wxEditTextCtrl::OnChar)
    EVT_KEY_UP         (wxEditTextCtrl::OnKeyUp)
    EVT_KILL_FOCUS     (wxEditTextCtrl::OnKillFocus)
END_EVENT_TABLE()

wxEditTextCtrl::wxEditTextCtrl (wxWindow *parent,
                                const wxWindowID id,
                                bool *accept,
                                wxString *res,
                                wxTreeListMainWindow *owner,
                                const wxString &value,
                                const wxPoint &pos,
                                const wxSize &size,
                                long style,
                                const wxValidator& validator,
                                const wxString &name)
    : wxTextCtrl (parent, id, value, pos, size, style | wxSIMPLE_BORDER | wxTE_PROCESS_ENTER, validator, name)
{
    m_res = res;
    m_accept = accept;
    m_owner = owner;
    (*m_accept) = false;
    (*m_res) = wxEmptyString;
    m_startValue = value;
    m_finished = false;
}

wxEditTextCtrl::~wxEditTextCtrl() {
    EndEdit(true); // cancelled
}

void wxEditTextCtrl::EndEdit(bool isCancelled) {
    if (m_finished) return;
    m_finished = true;

    if (m_owner) {
        (*m_accept) = ! isCancelled;
        (*m_res) = isCancelled ? m_startValue : GetValue();
        m_owner->OnRenameAccept(*m_res == m_startValue);
        m_owner->m_editControl = NULL;
        m_owner->m_editItem = NULL;
        m_owner->SetFocus(); // This doesn't work. TODO.
        m_owner = NULL;
    }

    Destroy();
}

bool wxEditTextCtrl::Destroy() {
    Hide();
    wxTheApp->ScheduleForDestruction(this);
    return true;
}

void wxEditTextCtrl::OnChar( wxKeyEvent &event )
{
    if (m_finished)
    {
        event.Skip();
        return;
    }
    if (event.GetKeyCode() == WXK_RETURN)
    {
        EndEdit(false);  // not cancelled
        return;
    }
    if (event.GetKeyCode() == WXK_ESCAPE)
    {
        EndEdit(true);  // cancelled
        return;
    }
    event.Skip();
}

void wxEditTextCtrl::OnKeyUp( wxKeyEvent &event )
{
    if (m_finished)
    {
        event.Skip();
        return;
    }

    // auto-grow the textctrl:
    wxSize parentSize = m_owner->GetSize();
    wxPoint myPos = GetPosition();
    wxSize mySize = GetSize();
    int sx, sy;
    GetTextExtent(GetValue() + _T("M"), &sx, &sy);
    if (myPos.x + sx > parentSize.x) sx = parentSize.x - myPos.x;
    if (mySize.x > sx) sx = mySize.x;
    SetSize(sx, -1);

    event.Skip();
}

void wxEditTextCtrl::OnKillFocus( wxFocusEvent &event )
{
    if (m_finished)
    {
        event.Skip();
        return;
    }

    EndEdit(false);  // not cancelled
}

//-----------------------------------------------------------------------------
//  wxTreeListHeaderWindow
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxTreeListHeaderWindow,wxWindow);

BEGIN_EVENT_TABLE(wxTreeListHeaderWindow,wxWindow)
    EVT_PAINT         (wxTreeListHeaderWindow::OnPaint)
    EVT_ERASE_BACKGROUND(wxTreeListHeaderWindow::OnEraseBackground) // reduce flicker
    EVT_MOUSE_EVENTS  (wxTreeListHeaderWindow::OnMouse)
    EVT_SET_FOCUS     (wxTreeListHeaderWindow::OnSetFocus)
END_EVENT_TABLE()


void wxTreeListHeaderWindow::Init()
{
    m_currentCursor = (wxCursor *) NULL;
    m_isDragging = false;
    m_dirty = false;
    m_total_col_width = 0;
    m_hotTrackCol = -1;

    // prevent any background repaint in order to reducing flicker
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
}

wxTreeListHeaderWindow::wxTreeListHeaderWindow()
{
    Init();

    m_owner = (wxTreeListMainWindow *) NULL;
    m_resizeCursor = (wxCursor *) NULL;
}

wxTreeListHeaderWindow::wxTreeListHeaderWindow( wxWindow *win,
                                                wxWindowID id,
                                                wxTreeListMainWindow *owner,
                                                const wxPoint& pos,
                                                const wxSize& size,
                                                long style,
                                                const wxString &name )
    : wxWindow( win, id, pos, size, style, name )
{
    Init();

    m_owner = owner;
    m_resizeCursor = new wxCursor(wxCURSOR_SIZEWE);

    SetBackgroundColour (wxSystemSettings::GetColour (wxSYS_COLOUR_BTNFACE));
}

wxTreeListHeaderWindow::~wxTreeListHeaderWindow()
{
    delete m_resizeCursor;
}

void wxTreeListHeaderWindow::DoDrawRect( wxDC *dc, int x, int y, int w, int h )
{
    wxPen pen (wxSystemSettings::GetColour (wxSYS_COLOUR_BTNSHADOW ), 1, wxPENSTYLE_SOLID);
    const int m_corner = 1;

    dc->SetBrush( *wxTRANSPARENT_BRUSH );
#if defined( __WXMAC__  )
    dc->SetPen (pen);
#else // !GTK, !Mac
    dc->SetPen( *wxBLACK_PEN );
#endif
    dc->DrawLine( x+w-m_corner+1, y, x+w, y+h );  // right (outer)
    dc->DrawRectangle( x, y+h, w+1, 1 );          // bottom (outer)

#if defined( __WXMAC__  )
    pen = wxPen( wxColour( 0x88 , 0x88 , 0x88 ), 1, wxSOLID );
#endif
    dc->SetPen( pen );
    dc->DrawLine( x+w-m_corner, y, x+w-1, y+h );  // right (inner)
    dc->DrawRectangle( x+1, y+h-1, w-2, 1 );      // bottom (inner)

    dc->SetPen( *wxWHITE_PEN );
    dc->DrawRectangle( x, y, w-m_corner+1, 1 );   // top (outer)
    dc->DrawRectangle( x, y, 1, h );              // left (outer)
    dc->DrawLine( x, y+h-1, x+1, y+h-1 );
    dc->DrawLine( x+w-1, y, x+w-1, y+1 );
}

// shift the DC origin to match the position of the main window horz
// scrollbar: this allows us to always use logical coords
void wxTreeListHeaderWindow::AdjustDC(wxDC& dc)
{
    int xpix;
    m_owner->GetScrollPixelsPerUnit( &xpix, NULL );
    int x;
    m_owner->GetViewStart( &x, NULL );

    // account for the horz scrollbar offset
    dc.SetDeviceOrigin( -x * xpix, 0 );
}

void wxTreeListHeaderWindow::OnPaint( wxPaintEvent &WXUNUSED(event) )
{
    wxAutoBufferedPaintDC dc( this );
    AdjustDC( dc );

    int x = HEADER_OFFSET_X;

    // width and height of the entire header window
    int w, h;
    GetClientSize( &w, &h );
    m_owner->CalcUnscrolledPosition(w, 0, &w, NULL);
    dc.SetBackgroundMode(wxTRANSPARENT);

    int numColumns = GetColumnCount();
    for ( int i = 0; i < numColumns && x < w; i++ )
    {
        if (!IsColumnShown (i)) continue; // do next column if not shown

        wxHeaderButtonParams params;

        // TODO: columnInfo should have label colours...
        params.m_labelColour = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );
        params.m_labelFont = GetFont();

        wxTreeListColumnInfo& column = GetColumn(i);
        int wCol = column.GetWidth();
        int flags = 0;
        wxRect rect(x, 0, wCol, h);
        x += wCol;

        if ( i == m_hotTrackCol)
            flags |= wxCONTROL_CURRENT;

        params.m_labelText = column.GetText();
        params.m_labelAlignment = column.GetAlignment();

        int image = column.GetImage();
        wxImageList* imageList = m_owner->GetImageList();
        if ((image != -1) && imageList)
            params.m_labelBitmap = imageList->GetBitmap(image);

        wxRendererNative::Get().DrawHeaderButton(this, dc, rect, flags, wxHDR_SORT_ICON_NONE, &params);
    }

    if (x < w) {
        wxRect rect(x, 0, w-x, h);
        wxRendererNative::Get().DrawHeaderButton(this, dc, rect);
    }

}

void wxTreeListHeaderWindow::DrawCurrent()
{
    int x1 = m_currentX;
    int y1 = 0;
    ClientToScreen (&x1, &y1);

    int x2 = m_currentX-1;
#ifdef __WXMSW__
    ++x2; // but why ????
#endif
    int y2 = 0;
    m_owner->GetClientSize( NULL, &y2 );
    m_owner->ClientToScreen( &x2, &y2 );

    wxScreenDC dc;
    dc.SetLogicalFunction (wxINVERT);
    dc.SetPen (wxPen (*wxBLACK, 2, wxPENSTYLE_SOLID));
    dc.SetBrush (*wxTRANSPARENT_BRUSH);

    AdjustDC(dc);
    dc.DrawLine (x1, y1, x2, y2);
    dc.SetLogicalFunction (wxCOPY);
    dc.SetPen (wxNullPen);
    dc.SetBrush (wxNullBrush);
}

int wxTreeListHeaderWindow::XToCol(int x)
{
    int colLeft = 0;
    int numColumns = GetColumnCount();
    for ( int col = 0; col < numColumns; col++ )
    {
        if (!IsColumnShown(col)) continue;
        wxTreeListColumnInfo& column = GetColumn(col);

        if ( x < (colLeft + column.GetWidth()) )
             return col;

        colLeft += column.GetWidth();
    }
    return -1;
}

void wxTreeListHeaderWindow::RefreshColLabel(int col)
{
    if ( col > GetColumnCount() )
        return;

    int x = 0;
    int width = 0;
    int idx = 0;
    do {
        if (!IsColumnShown(idx)) continue;
        wxTreeListColumnInfo& column = GetColumn(idx);
        x += width;
        width = column.GetWidth();
    } while (++idx <= col);

    m_owner->CalcScrolledPosition(x, 0, &x, NULL);
    RefreshRect(wxRect(x, 0, width, GetSize().GetHeight()));
}

void wxTreeListHeaderWindow::OnMouse (wxMouseEvent &event) {

    // we want to work with logical coords
    int x;
    m_owner->CalcUnscrolledPosition(event.GetX(), 0, &x, NULL);

    const int col = XToCol(x);
    if(col>=0 && col<GetColumnCount())
    {
        SetToolTip(m_columns[col].GetTooltip());
    }
    else
    {
        SetToolTip(wxEmptyString);
    };
    if ( event.Moving() )
    {
        if ( col != m_hotTrackCol )
        {
            // Refresh the col header so it will be painted with hot tracking
            // (if supported by the native renderer.)
            RefreshColLabel(col);

            // Also refresh the old hot header
            if ( m_hotTrackCol >= 0 )
                RefreshColLabel(m_hotTrackCol);

            m_hotTrackCol = col;
        }
    }

    if ( event.Leaving() && m_hotTrackCol >= 0 )
    {
        // Leaving the window so clear any hot tracking indicator that may be present
        RefreshColLabel(m_hotTrackCol);
        m_hotTrackCol = -1;
    }

    if (m_isDragging) {

        SendListEvent (wxEVT_COMMAND_LIST_COL_DRAGGING, event.GetPosition());

        // we don't draw the line beyond our window, but we allow dragging it
        // there
        int w = 0;
        GetClientSize( &w, NULL );
        m_owner->CalcUnscrolledPosition(w, 0, &w, NULL);
        w -= 6;

        // erase the line if it was drawn
        if (m_currentX < w) DrawCurrent();

        if (event.ButtonUp()) {
            m_isDragging = false;
            if (HasCapture()) ReleaseMouse();
            m_dirty = true;
            SetColumnWidth (m_column, m_currentX - m_minX);
            Refresh();
            SendListEvent (wxEVT_COMMAND_LIST_COL_END_DRAG, event.GetPosition());
        }else{
            m_currentX = wxMax (m_minX + 7, x);

            // draw in the new location
            if (m_currentX < w) DrawCurrent();
        }

    }else{ // not dragging

        m_minX = 0;
        bool hit_border = false;

        // end of the current column
        int xpos = 0;

        // find the column where this event occurred
        int countCol = GetColumnCount();
        for (int column = 0; column < countCol; column++) {
            if (!IsColumnShown (column)) continue; // do next if not shown

            xpos += GetColumnWidth (column);
            m_column = column;
            if (abs (x-xpos) < 3) {
                // near the column border
                hit_border = true;
                break;
            }

            if (x < xpos) {
                // inside the column
                break;
            }

            m_minX = xpos;
        }

        if (event.LeftDown() || event.RightUp()) {
            m_owner->EndEdit(true);  // cancelled

            if (hit_border && event.LeftDown()) {
                m_isDragging = true;
                CaptureMouse();
                m_currentX = x;
                DrawCurrent();
                SendListEvent (wxEVT_COMMAND_LIST_COL_BEGIN_DRAG, event.GetPosition());
            }else{ // click on a column
                wxEventType evt = event.LeftDown()? wxEVT_COMMAND_LIST_COL_CLICK:
                                                    wxEVT_COMMAND_LIST_COL_RIGHT_CLICK;
                SendListEvent (evt, event.GetPosition());
            }
        }else if (event.LeftDClick() && hit_border) {
            SetColumnWidth (m_column, m_owner->GetBestColumnWidth (m_column));
            Refresh();

        }else if (event.Moving()) {
            bool setCursor;
            if (hit_border) {
                setCursor = m_currentCursor == wxSTANDARD_CURSOR;
                m_currentCursor = m_resizeCursor;
            }else{
                setCursor = m_currentCursor != wxSTANDARD_CURSOR;
                m_currentCursor = wxSTANDARD_CURSOR;
            }
            if (setCursor) SetCursor (*m_currentCursor);
        }

    }
}

void wxTreeListHeaderWindow::OnSetFocus (wxFocusEvent &WXUNUSED(event)) {
    m_owner->SetFocus();
}

void wxTreeListHeaderWindow::SendListEvent (wxEventType type, wxPoint pos) {
    wxWindow *parent = GetParent();
    wxListEvent le (type, parent->GetId());
    le.SetEventObject (parent);
    le.m_pointDrag = pos;

    // the position should be relative to the parent window, not
    // this one for compatibility with MSW and common sense: the
    // user code doesn't know anything at all about this header
    // window, so why should it get positions relative to it?
    le.m_pointDrag.y -= GetSize().y;
    le.m_col = m_column;
    parent->GetEventHandler()->ProcessEvent (le);
}

void wxTreeListHeaderWindow::AddColumn (const wxTreeListColumnInfo& colInfo) {
    m_columns.Add (colInfo);
    m_total_col_width += colInfo.GetWidth();
    m_owner->AdjustMyScrollbars();
    m_owner->m_dirty = true;
}

void wxTreeListHeaderWindow::SetColumnWidth (int column, int width) {
    wxCHECK_RET ((column >= 0) && (column < GetColumnCount()), _T("Invalid column"));
    m_total_col_width -= m_columns[column].GetWidth();
    m_columns[column].SetWidth(width);
    m_total_col_width += m_columns[column].GetWidth();
    m_owner->AdjustMyScrollbars();
    m_owner->m_dirty = true;
}

void wxTreeListHeaderWindow::InsertColumn (int before, const wxTreeListColumnInfo& colInfo) {
    wxCHECK_RET ((before >= 0) && (before < GetColumnCount()), _T("Invalid column"));
    m_columns.Insert (colInfo, before);
    m_total_col_width += colInfo.GetWidth();
    m_owner->AdjustMyScrollbars();
    m_owner->m_dirty = true;
}

void wxTreeListHeaderWindow::RemoveColumn (int column) {
    wxCHECK_RET ((column >= 0) && (column < GetColumnCount()), _T("Invalid column"));
    m_total_col_width -= m_columns[column].GetWidth();
    m_columns.RemoveAt (column);
    m_owner->AdjustMyScrollbars();
    m_owner->m_dirty = true;
}

void wxTreeListHeaderWindow::SetColumn (int column, const wxTreeListColumnInfo& info) {
    wxCHECK_RET ((column >= 0) && (column < GetColumnCount()), _T("Invalid column"));
    int w = m_columns[column].GetWidth();
    m_columns[column] = info;
    if (w != info.GetWidth()) {
        m_total_col_width -= w;
        m_total_col_width += info.GetWidth();
        m_owner->AdjustMyScrollbars();
    }
    m_owner->m_dirty = true;
}

// ---------------------------------------------------------------------------
// wxTreeListItem
// ---------------------------------------------------------------------------

wxTreeListItem::wxTreeListItem (wxTreeListMainWindow *owner,
                                wxTreeListItem *parent,
                                const wxArrayString& text,
                                int image, int selImage,
                                wxTreeItemData *data)
              : m_text (text) {

    m_images[wxTreeItemIcon_Normal] = image;
    m_images[wxTreeItemIcon_Selected] = selImage;
    m_images[wxTreeItemIcon_Expanded] = NO_IMAGE;
    m_images[wxTreeItemIcon_SelectedExpanded] = NO_IMAGE;

    m_props_row.m_data = data;
    m_toolTip = NULL;
    m_x = 0;
    m_y = 0;
    m_text_x.resize(m_text.GetCount(), 0);

    m_isCollapsed = true;
    m_hasHilight = false;
    m_hasPlus = false;

    m_owner = owner;
    m_parent = parent;

    // We don't know the height here yet.
    m_width = 0;
    m_height = 0;
}

wxTreeListItem::~wxTreeListItem() {
    if (m_toolTip) delete m_toolTip;

    wxTreeListItemCellAttrHash::iterator entry = m_props_cell.begin();
    while (entry != m_props_cell.end()) {
        if (entry->second) delete entry->second;
        ++entry;
    }

    wxASSERT_MSG( m_children.IsEmpty(), _T("please call DeleteChildren() before destructor"));
}

void wxTreeListItem::DeleteChildren () {
    m_children.Empty();
}

size_t wxTreeListItem::GetChildrenCount (bool recursively) const {
    size_t count = m_children.Count();
    if (!recursively) return count;

    size_t total = count;
    for (size_t n = 0; n < count; ++n) {
        total += m_children[n]->GetChildrenCount();
    }
    return total;
}

void wxTreeListItem::GetSize (int &x, int &y, const wxTreeListMainWindow *theButton) {
    int bottomY = m_y + theButton->GetLineHeight (this);
    if (y < bottomY) y = bottomY;
    int width = m_x +  GetWidth();
    if ( x < width ) x = width;

    if (IsExpanded()) {
        size_t count = m_children.Count();
        for (size_t n = 0; n < count; ++n ) {
            m_children[n]->GetSize (x, y, theButton);
        }
    }
}

wxTreeListItem *wxTreeListItem::HitTest (const wxPoint& point,
                                         const wxTreeListMainWindow *theCtrl,
                                         int &flags, int& column, int level) {

    // reset any previous hit infos
    flags = 0;
    column = -1;

    // for a hidden root node, don't evaluate it, but do evaluate children
    if (!theCtrl->HasFlag(wxTR_HIDE_ROOT) || (level > 0)) {

        wxTreeListHeaderWindow* header_win = theCtrl->m_owner->GetHeaderWindow();

        // check for right of all columns (outside)
        if (point.x > header_win->GetWidth()) return (wxTreeListItem*) NULL;
        // else find column
        for (int x = 0, j = 0; j < theCtrl->GetColumnCount(); ++j) {
            if (!header_win->IsColumnShown(j)) continue;
            int w = header_win->GetColumnWidth (j);
            if (point.x >= x && point.x < x+w) {
                column = j;
                break;
            }
            x += w;
        }

        // evaluate if y-pos is okay
        int h = theCtrl->GetLineHeight (this);
        if ((point.y >= m_y) && (point.y <= m_y + h)) {

            // check for above/below middle
            int y_mid = m_y + h/2;
            if (point.y < y_mid) {
                flags |= wxTREE_HITTEST_ONITEMUPPERPART;
            }else{
                flags |= wxTREE_HITTEST_ONITEMLOWERPART;
            }

            // check for button hit
            if (HasPlus() && theCtrl->HasButtons()) {
                int bntX = m_x - theCtrl->m_btnWidth2;
                int bntY = y_mid - theCtrl->m_btnHeight2;
                if ((point.x >= bntX) && (point.x <= (bntX + theCtrl->m_btnWidth)) &&
                    (point.y >= bntY) && (point.y <= (bntY + theCtrl->m_btnHeight))) {
                    flags |= wxTREE_HITTEST_ONITEMBUTTON;
                    return this;
                }
            }

            // check for image hit
            if (theCtrl->m_imgWidth > 0) {
                int imgX = GetTextX(column) - theCtrl->m_imgWidth - MARGIN;
                int imgY = y_mid - theCtrl->m_imgHeight2;
                if ((point.x >= imgX) && (point.x <= (imgX + theCtrl->m_imgWidth)) &&
                    (point.y >= imgY) && (point.y <= (imgY + theCtrl->m_imgHeight))) {
                    flags |= wxTREE_HITTEST_ONITEMICON;
                    return this;
                }
            }

            // check for label hit
            if ((point.x >= GetTextX(column)) && (point.x <= (GetTextX(column) + GetWidth()))) {
                flags |= wxTREE_HITTEST_ONITEMLABEL;
                return this;
            }

            // check for indent hit after button and image hit
            if (point.x < m_x) {
                flags |= wxTREE_HITTEST_ONITEMINDENT;
                return this;
            }

            // check for right of label
            int end = 0;
            for (int i = 0; i <= theCtrl->GetMainColumn(); ++i) end += header_win->GetColumnWidth (i);
            if ((point.x > (GetTextX(column) + GetWidth())) && (point.x <= end)) {
                flags |= wxTREE_HITTEST_ONITEMRIGHT;
                return this;
            }

            // else check for each column except main
            if (column >= 0 && column != theCtrl->GetMainColumn()) {
                flags |= wxTREE_HITTEST_ONITEMCOLUMN;
                return this;
            }

            // no special flag or column found
            return this;

        }

        // if children not expanded, return no item
        if (!IsExpanded()) return (wxTreeListItem*) NULL;
    }

    // in any case evaluate children
    wxTreeListItem *child;
    size_t count = m_children.Count();
    for (size_t n = 0; n < count; n++) {
        child = m_children[n]->HitTest (point, theCtrl, flags, column, level+1);
        if (child) return child;
    }

    // not found
    return (wxTreeListItem*) NULL;
}

int wxTreeListItem::GetCurrentImage() const {
    int image = NO_IMAGE;
    if (IsExpanded()) {
        if (IsSelected()) {
            image = GetImage (wxTreeItemIcon_SelectedExpanded);
        }else{
            image = GetImage (wxTreeItemIcon_Expanded);
        }
    }else{ // not expanded
        if (IsSelected()) {
            image = GetImage (wxTreeItemIcon_Selected);
        }else{
            image = GetImage (wxTreeItemIcon_Normal);
        }
    }

    // maybe it doesn't have the specific image, try the default one instead
    if (image == NO_IMAGE) image = GetImage();

    return image;
}

// ---------------------------------------------------------------------------
// wxTreeListMainWindow implementation
// ---------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxTreeListMainWindow, wxScrolledWindow)

BEGIN_EVENT_TABLE(wxTreeListMainWindow, wxScrolledWindow)
    EVT_PAINT          (wxTreeListMainWindow::OnPaint)
    EVT_ERASE_BACKGROUND(wxTreeListMainWindow::OnEraseBackground) // to reduce flicker
    EVT_MOUSE_EVENTS   (wxTreeListMainWindow::OnMouse)
    EVT_CHAR           (wxTreeListMainWindow::OnChar)
    EVT_SET_FOCUS      (wxTreeListMainWindow::OnSetFocus)
    EVT_KILL_FOCUS     (wxTreeListMainWindow::OnKillFocus)
    EVT_IDLE           (wxTreeListMainWindow::OnIdle)
    EVT_SCROLLWIN      (wxTreeListMainWindow::OnScroll)
    EVT_MOUSE_CAPTURE_LOST(wxTreeListMainWindow::OnCaptureLost)
#if wxCHECK_VERSION(3,1,3)
    EVT_DPI_CHANGED(wxTreeListMainWindow::OnDpiChanged)
#endif
END_EVENT_TABLE()


// ---------------------------------------------------------------------------
// construction/destruction
// ---------------------------------------------------------------------------


void wxTreeListMainWindow::Init() {

    m_rootItem = (wxTreeListItem*)NULL;
    m_curItem = (wxTreeListItem*)NULL;
    m_shiftItem = (wxTreeListItem*)NULL;
    m_editItem = (wxTreeListItem*)NULL;
    m_selectItem = (wxTreeListItem*)NULL;

    m_curColumn = -1; // no current column

    m_hasFocus = false;
    m_dirty = false;

    m_lineHeight = LINEHEIGHT;
    m_indent = MININDENT; // min. indent
    m_linespacing = 4;

    m_hilightBrush = new wxBrush (wxSystemSettings::GetColour (wxSYS_COLOUR_HIGHLIGHT), wxBRUSHSTYLE_SOLID);
    m_hilightUnfocusedBrush = new wxBrush (wxSystemSettings::GetColour (wxSYS_COLOUR_BTNSHADOW), wxBRUSHSTYLE_SOLID);

    m_imageListNormal = (wxImageList *) NULL;
    m_imageListButtons = (wxImageList *) NULL;
    m_imageListState = (wxImageList *) NULL;
    m_ownsImageListNormal = m_ownsImageListButtons =
    m_ownsImageListState = false;

    m_imgWidth = 0, m_imgWidth2 = 0;
    m_imgHeight = 0, m_imgHeight2 = 0;
    m_btnWidth = 0, m_btnWidth2 = 0;
    m_btnHeight = 0, m_btnHeight2 = 0;

    m_isDragStarted = m_isDragging = false;
    m_dragItem = NULL;
    m_dragCol = -1;

    m_editTimer = new wxTreeListRenameTimer (this);
    m_editControl = NULL;

    m_lastOnSame = false;
    m_left_down_selection = false;

    m_findTimer = new wxTimer (this, -1);

#if defined( __WXMAC__ ) && defined(__WXMAC_CARBON__)
    m_normalFont.MacCreateThemeFont (kThemeViewsFont);
#else
    m_normalFont = wxSystemSettings::GetFont (wxSYS_DEFAULT_GUI_FONT);
#endif
    m_boldFont = wxFont( m_normalFont.GetPointSize(),
                         m_normalFont.GetFamily(),
                         m_normalFont.GetStyle(),
                         wxFONTWEIGHT_BOLD,
                         m_normalFont.GetUnderlined(),
                         m_normalFont.GetFaceName(),
                         m_normalFont.GetEncoding());
    m_toolTip.clear();
    m_toolTipItem = (wxTreeListItem *)-1;  // no tooltip displayed
    m_isItemToolTip = false;  // so far no item-specific tooltip
}

bool wxTreeListMainWindow::Create (wxTreeListCtrl *parent,
                                   wxWindowID id,
                                   const wxPoint& pos,
                                   const wxSize& size,
                                   long style,
                                   const wxValidator &validator,
                                   const wxString& name) {

    wxScrolledWindow::Create (parent, id, pos, size, style|wxHSCROLL|wxVSCROLL, name);

#if wxUSE_VALIDATORS
    SetValidator(validator);
#endif

    SetBackgroundColour (wxSystemSettings::GetColour (wxSYS_COLOUR_LISTBOX));
    // prevent any background repaint in order to reducing flicker
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);

    m_dottedPen = wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT), 0, wxPENSTYLE_DOT);

    m_owner = parent;
    m_main_column = 0;

    return true;
}

wxTreeListMainWindow::~wxTreeListMainWindow() {
    delete m_hilightBrush;
    delete m_hilightUnfocusedBrush;

    delete m_editTimer;
    delete m_findTimer;
    if (m_ownsImageListNormal) delete m_imageListNormal;
    if (m_ownsImageListState) delete m_imageListState;
    if (m_ownsImageListButtons) delete m_imageListButtons;

    if (m_editControl) {
        m_editControl->SetOwner(NULL);    // prevent control from calling us during delete
        delete m_editControl;
    }

    DeleteRoot();
}


//-----------------------------------------------------------------------------
// accessors
//-----------------------------------------------------------------------------

size_t wxTreeListMainWindow::GetCount() const {
    return m_rootItem == NULL? 0: m_rootItem->GetChildrenCount();
}

void wxTreeListMainWindow::SetIndent (unsigned int indent) {
    m_indent = wxMax ((unsigned)MININDENT, indent);
    m_dirty = true;
}

void wxTreeListMainWindow::SetLineSpacing (unsigned int spacing) {
    m_linespacing = spacing;
    m_dirty = true;
    CalculateLineHeight();
}

size_t wxTreeListMainWindow::GetChildrenCount (const wxTreeItemId& item,
                                               bool recursively) {
    wxCHECK_MSG (item.IsOk(), 0u, _T("invalid tree item"));
    return ((wxTreeListItem*)item.m_pItem)->GetChildrenCount (recursively);
}

void wxTreeListMainWindow::SetWindowStyle (const long styles) {
    // change to selection mode, reset selection
    if ((styles ^ m_windowStyle) & wxTR_MULTIPLE) { UnselectAll(); }
    // right now, just sets the styles.  Eventually, we may
    // want to update the inherited styles, but right now
    // none of the parents has updatable styles
    m_windowStyle = styles;
    m_dirty = true;
}

void wxTreeListMainWindow::SetToolTip(const wxString& tip) {
    m_isItemToolTip = false;
    m_toolTip = tip;
    m_toolTipItem = (wxTreeListItem *)-1;  // no tooltip displayed (force refresh)
}
void wxTreeListMainWindow::SetToolTip(wxToolTip *tip) {
    m_isItemToolTip = false;
    m_toolTip = (tip == NULL) ? wxString() : tip->GetTip();
    m_toolTipItem = (wxTreeListItem *)-1;  // no tooltip displayed (force refresh)
}

void wxTreeListMainWindow::SetItemToolTip(const wxTreeItemId& item, const wxString &tip) {
    wxCHECK_RET (item.IsOk(), _T("invalid tree item"));
    m_isItemToolTip = true;
    ((wxTreeListItem*) item.m_pItem)->SetToolTip(tip);
    m_toolTipItem = (wxTreeListItem *)-1;  // no tooltip displayed (force refresh)
}


//-----------------------------------------------------------------------------
// functions to work with tree items
//-----------------------------------------------------------------------------

int wxTreeListMainWindow::GetItemImage (const wxTreeItemId& item, int column, wxTreeItemIcon which) const {
    wxCHECK_MSG (item.IsOk(), -1, _T("invalid tree item"));
    return ((wxTreeListItem*) item.m_pItem)->GetImage (column, which);
}

wxTreeItemData *wxTreeListMainWindow::GetItemData (const wxTreeItemId& item) const {
    wxCHECK_MSG (item.IsOk(), NULL, _T("invalid tree item"));
    return ((wxTreeListItem*) item.m_pItem)->GetData();
}
wxTreeItemData *wxTreeListMainWindow::GetItemData (const wxTreeItemId& item, int column) const {
    wxCHECK_MSG (item.IsOk(), NULL, _T("invalid tree item"));
    return ((wxTreeListItem*) item.m_pItem)->GetData(column);
}

bool wxTreeListMainWindow::GetItemBold (const wxTreeItemId& item) const {
    wxCHECK_MSG(item.IsOk(), false, _T("invalid tree item"));
    return ((wxTreeListItem *)item.m_pItem)->IsBold();
}
bool wxTreeListMainWindow::GetItemBold (const wxTreeItemId& item, int column) const {
    wxCHECK_MSG(item.IsOk(), false, _T("invalid tree item"));
    return ((wxTreeListItem *)item.m_pItem)->IsBold(column);
}

wxColour wxTreeListMainWindow::GetItemTextColour (const wxTreeItemId& item) const {
    wxCHECK_MSG (item.IsOk(), wxNullColour, _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    wxTreeItemAttr *attr = pItem->GetAttributes();
    if (attr && attr->HasTextColour()) {
        return attr->GetTextColour();
    } else {
        return GetForegroundColour();
    }
}
wxColour wxTreeListMainWindow::GetItemTextColour (const wxTreeItemId& item, int column) const {
    wxCHECK_MSG (item.IsOk(), wxNullColour, _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    wxTreeItemAttr *attr = pItem->GetAttributes(column);
    if (attr && attr->HasTextColour()) {
        return attr->GetTextColour();
    } else {
        return GetItemTextColour(item);
    }
}

wxColour wxTreeListMainWindow::GetItemBackgroundColour (const wxTreeItemId& item) const {
    wxCHECK_MSG (item.IsOk(), wxNullColour, _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    wxTreeItemAttr *attr = pItem->GetAttributes();
    if (attr && attr->HasBackgroundColour()) {
        return attr->GetBackgroundColour();
    } else {
        return GetBackgroundColour();
    }
}
wxColour wxTreeListMainWindow::GetItemBackgroundColour (const wxTreeItemId& item, int column) const {
    wxCHECK_MSG (item.IsOk(), wxNullColour, _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    wxTreeItemAttr *attr = pItem->GetAttributes(column);
    if (attr && attr->HasBackgroundColour()) {
        return attr->GetBackgroundColour();
    } else {
        return GetItemBackgroundColour(item);
    }
}

wxFont wxTreeListMainWindow::GetItemFont (const wxTreeItemId& item) const {
    wxCHECK_MSG (item.IsOk(), wxNullFont, _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    wxTreeItemAttr *attr = pItem->GetAttributes();
    if (attr && attr->HasFont()) {
        return attr->GetFont();
    }else if (pItem->IsBold()) {
        return m_boldFont;
    } else {
        return m_normalFont;
    }
}
wxFont wxTreeListMainWindow::GetItemFont (const wxTreeItemId& item, int column) const {
    wxCHECK_MSG (item.IsOk(), wxNullFont, _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    wxTreeItemAttr *attr_cell = pItem->GetAttributes(column);
    wxTreeItemAttr *attr_row = pItem->GetAttributes();
    if (attr_cell && attr_cell->HasFont()) {
        return attr_cell->GetFont();
    } else if (attr_row && attr_row->HasFont()) {
        return attr_row->GetFont();
    } else if (pItem->IsBold(column)) {
        return m_boldFont;
    } else {
        return m_normalFont;
    }
}

void wxTreeListMainWindow::SetItemHasChildren (const wxTreeItemId& item, bool has) {
    wxCHECK_RET (item.IsOk(), _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    pItem->SetHasPlus (has);
    RefreshLine (pItem);
}

void wxTreeListMainWindow::SetItemImage (const wxTreeItemId& item, int column, int image, wxTreeItemIcon which) {
    wxCHECK_RET (item.IsOk(), _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    if(pItem->GetImage(column, which) != image)
    {
      pItem->SetImage (column, image, which);
      if(!IsFrozen())
      {
        wxClientDC dc (this);
        CalculateSize (pItem, dc);
        RefreshLine (pItem);
      };
    };
}

void wxTreeListMainWindow::SetItemData (const wxTreeItemId& item,             wxTreeItemData *data) {
    wxCHECK_RET (item.IsOk(), _T("invalid tree item"));
    ((wxTreeListItem*) item.m_pItem)->SetData(data);
}
void wxTreeListMainWindow::SetItemData (const wxTreeItemId& item, int column, wxTreeItemData *data) {
    wxCHECK_RET (item.IsOk(), _T("invalid tree item"));
    ((wxTreeListItem*) item.m_pItem)->SetData(column, data);
}

void wxTreeListMainWindow::SetItemBold (const wxTreeItemId& item,             bool bold) {
    wxCHECK_RET (item.IsOk(), _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    if (pItem->IsBold() != bold) { // avoid redrawing if no real change
        pItem->SetBold (bold);
        RefreshLine (pItem);
    }
}
void wxTreeListMainWindow::SetItemBold (const wxTreeItemId& item, int column, bool bold) {
    wxCHECK_RET (item.IsOk(), _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
//    if (pItem->IsBold(column) != bold) { // avoid redrawing if no real change
        pItem->SetBold (column, bold);
        RefreshLine (pItem);
//    }
}

void wxTreeListMainWindow::SetItemTextColour (const wxTreeItemId& item,             const wxColour& colour) {
    wxCHECK_RET (item.IsOk(), _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    pItem->Attr().SetTextColour (colour);
    RefreshLine (pItem);
}
void wxTreeListMainWindow::SetItemTextColour (const wxTreeItemId& item, int column, const wxColour& colour) {
    wxCHECK_RET (item.IsOk(), _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    pItem->Attr(column).SetTextColour (colour);
    RefreshLine (pItem);
}

void wxTreeListMainWindow::SetItemBackgroundColour (const wxTreeItemId& item,             const wxColour& colour) {
    wxCHECK_RET (item.IsOk(), _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    pItem->Attr().SetBackgroundColour (colour);
    RefreshLine (pItem);
}
void wxTreeListMainWindow::SetItemBackgroundColour (const wxTreeItemId& item, int column, const wxColour& colour) {
    wxCHECK_RET (item.IsOk(), _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    pItem->Attr(column).SetBackgroundColour (colour);
    RefreshLine (pItem);
}

void wxTreeListMainWindow::SetItemFont (const wxTreeItemId& item,             const wxFont& font) {
    wxCHECK_RET (item.IsOk(), _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    pItem->Attr().SetFont (font);
    RefreshLine (pItem);
}
void wxTreeListMainWindow::SetItemFont (const wxTreeItemId& item, int column, const wxFont& font) {
    wxCHECK_RET (item.IsOk(), _T("invalid tree item"));
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    pItem->Attr(column).SetFont (font);
    RefreshLine (pItem);
}


bool wxTreeListMainWindow::SetFont (const wxFont &font) {
    wxScrolledWindow::SetFont (font);
    m_normalFont = font;
    m_boldFont = wxFont (m_normalFont.GetPointSize(),
                         m_normalFont.GetFamily(),
                         m_normalFont.GetStyle(),
                         wxFONTWEIGHT_BOLD,
                         m_normalFont.GetUnderlined(),
                         m_normalFont.GetFaceName());
    CalculateLineHeight();
    return true;
}


// ----------------------------------------------------------------------------
// item status inquiries
// ----------------------------------------------------------------------------

bool wxTreeListMainWindow::IsVisible (const wxTreeItemId& item, bool fullRow, bool within) const {
    wxCHECK_MSG (item.IsOk(), false, _T("invalid tree item"));

    // An item is only visible if it's not a descendant of a collapsed item
    wxTreeListItem *pItem = (wxTreeListItem*) item.m_pItem;
    wxTreeListItem* parent = pItem->GetItemParent();
    while (parent) {
        if (parent == m_rootItem && HasFlag(wxTR_HIDE_ROOT)) break;
        if (!parent->IsExpanded()) return false;
        parent = parent->GetItemParent();
    }

    // and the item is only visible if it is currently (fully) within the view
    if (within) {
        wxSize clientSize = GetClientSize();
        wxRect rect;
        if ((!GetBoundingRect (item, rect)) ||
            ((!fullRow && rect.GetWidth() == 0) || rect.GetHeight() == 0) ||
            (rect.GetTop() < 0 || rect.GetBottom() >= clientSize.y) ||
            (!fullRow && (rect.GetLeft() < 0 || rect.GetRight() >= clientSize.x))) return false;
    }

    return true;
}

bool wxTreeListMainWindow::HasChildren (const wxTreeItemId& item) const {
    wxCHECK_MSG (item.IsOk(), false, _T("invalid tree item"));

    // consider that the item does have children if it has the "+" button: it
    // might not have them (if it had never been expanded yet) but then it
    // could have them as well and it's better to err on this side rather than
    // disabling some operations which are restricted to the items with
    // children for an item which does have them
    return ((wxTreeListItem*) item.m_pItem)->HasPlus();
}

bool wxTreeListMainWindow::IsExpanded (const wxTreeItemId& item) const {
    wxCHECK_MSG (item.IsOk(), false, _T("invalid tree item"));
    return ((wxTreeListItem*) item.m_pItem)->IsExpanded();
}

bool wxTreeListMainWindow::IsSelected (const wxTreeItemId& item) const {
    wxCHECK_MSG (item.IsOk(), false, _T("invalid tree item"));
    return ((wxTreeListItem*) item.m_pItem)->IsSelected();
}

bool wxTreeListMainWindow::IsBold (const wxTreeItemId& item, int column) const {
    wxCHECK_MSG (item.IsOk(), false, _T("invalid tree item"));
    return ((wxTreeListItem*) item.m_pItem)->IsBold(column);
}

// ----------------------------------------------------------------------------
// navigation
// ----------------------------------------------------------------------------

wxTreeItemId wxTreeListMainWindow::GetItemParent (const wxTreeItemId& item) const {
    wxCHECK_MSG (item.IsOk(), wxTreeItemId(), _T("invalid tree item"));
    return ((wxTreeListItem*) item.m_pItem)->GetItemParent();
}

wxTreeItemId wxTreeListMainWindow::GetFirstChild (const wxTreeItemId& item,
                                                  wxTreeItemIdValue& cookie) const {
    wxCHECK_MSG (item.IsOk(), wxTreeItemId(), _T("invalid tree item"));
    wxArrayTreeListItems& children = ((wxTreeListItem*) item.m_pItem)->GetChildren();
    cookie = 0;
    return (!children.IsEmpty())? wxTreeItemId(children.Item(0)): wxTreeItemId();
}

wxTreeItemId wxTreeListMainWindow::GetNextChild (const wxTreeItemId& item,
                                                 wxTreeItemIdValue& cookie) const {
    wxCHECK_MSG (item.IsOk(), wxTreeItemId(), _T("invalid tree item"));
    wxArrayTreeListItems& children = ((wxTreeListItem*) item.m_pItem)->GetChildren();
    // it's ok to cast cookie to long, we never have indices which overflow "void*"
    long *pIndex = ((long*)&cookie);
    return ((*pIndex)+1 < (long)children.Count())? wxTreeItemId(children.Item(++(*pIndex))): wxTreeItemId();
}

wxTreeItemId wxTreeListMainWindow::GetPrevChild (const wxTreeItemId& item,
                                                 wxTreeItemIdValue& cookie) const {
    wxCHECK_MSG (item.IsOk(), wxTreeItemId(), _T("invalid tree item"));
    wxArrayTreeListItems& children = ((wxTreeListItem*) item.m_pItem)->GetChildren();
    // it's ok to cast cookie to long, we never have indices which overflow "void*"
    long *pIndex = (long*)&cookie;
    return ((*pIndex)-1 >= 0)? wxTreeItemId(children.Item(--(*pIndex))): wxTreeItemId();
}

wxTreeItemId wxTreeListMainWindow::GetLastChild (const wxTreeItemId& item,
                                                 wxTreeItemIdValue& cookie) const {
    wxCHECK_MSG (item.IsOk(), wxTreeItemId(), _T("invalid tree item"));
    wxArrayTreeListItems& children = ((wxTreeListItem*) item.m_pItem)->GetChildren();
    // it's ok to cast cookie to long, we never have indices which overflow "void*"
    long *pIndex = ((long*)&cookie);
    (*pIndex) = (long)children.Count();
    return (!children.IsEmpty())? wxTreeItemId(children.Last()): wxTreeItemId();
}

wxTreeItemId wxTreeListMainWindow::GetNextSibling (const wxTreeItemId& item) const {
    wxCHECK_MSG (item.IsOk(), wxTreeItemId(), _T("invalid tree item"));

    // get parent
    wxTreeListItem *i = (wxTreeListItem*) item.m_pItem;
    wxTreeListItem *parent = i->GetItemParent();
    if (!parent) return wxTreeItemId(); // root item doesn't have any siblings

    // get index
    wxArrayTreeListItems& siblings = parent->GetChildren();
    size_t index = siblings.Index (i);
    wxASSERT (index != (size_t)wxNOT_FOUND); // I'm not a child of my parent?
    return (index < siblings.Count()-1)? wxTreeItemId(siblings[index+1]): wxTreeItemId();
}

wxTreeItemId wxTreeListMainWindow::GetPrevSibling (const wxTreeItemId& item) const {
    wxCHECK_MSG (item.IsOk(), wxTreeItemId(), _T("invalid tree item"));

    // get parent
    wxTreeListItem *i = (wxTreeListItem*) item.m_pItem;
    wxTreeListItem *parent = i->GetItemParent();
    if (!parent) return wxTreeItemId(); // root item doesn't have any siblings

    // get index
    wxArrayTreeListItems& siblings = parent->GetChildren();
    size_t index = siblings.Index(i);
    wxASSERT (index != (size_t)wxNOT_FOUND); // I'm not a child of my parent?
    return (index >= 1)? wxTreeItemId(siblings[index-1]): wxTreeItemId();
}

// Only for internal use right now, but should probably be public
wxTreeItemId wxTreeListMainWindow::GetNext (const wxTreeItemId& item, bool fulltree) const {
    wxCHECK_MSG (item.IsOk(), wxTreeItemId(), _T("invalid tree item"));

    // if there are any children, return first child
    if (fulltree || ((wxTreeListItem*)item.m_pItem)->IsExpanded()) {
        wxArrayTreeListItems& children = ((wxTreeListItem*)item.m_pItem)->GetChildren();
        if (children.GetCount() > 0) return children.Item (0);
    }

    // get sibling of this item or of the ancestors instead
    wxTreeItemId next;
    wxTreeItemId parent = item;
    do {
        next = GetNextSibling (parent);
        parent = GetItemParent (parent);
    } while (!next.IsOk() && parent.IsOk());
    return next;
}

// Only for internal use right now, but should probably be public
wxTreeItemId wxTreeListMainWindow::GetPrev (const wxTreeItemId& item, bool fulltree) const {
    wxCHECK_MSG (item.IsOk(), wxTreeItemId(), _T("invalid tree item"));

    // if there are no previous sibling get parent
    wxTreeItemId prev = GetPrevSibling (item);
    if (! prev.IsOk()) return GetItemParent (item);

    // while previous sibling has children, return last
    while (fulltree || ((wxTreeListItem*)prev.m_pItem)->IsExpanded()) {
        wxArrayTreeListItems& children = ((wxTreeListItem*)prev.m_pItem)->GetChildren();
        if (children.GetCount() == 0) break;
        prev = children.Item (children.GetCount() - 1);
    }

    return prev;
}

wxTreeItemId wxTreeListMainWindow::GetFirstExpandedItem() const {
    return GetNextExpanded (GetRootItem());
}

wxTreeItemId wxTreeListMainWindow::GetNextExpanded (const wxTreeItemId& item) const {
    wxCHECK_MSG (item.IsOk(), wxTreeItemId(), _T("invalid tree item"));
    return GetNext (item, false);
}

wxTreeItemId wxTreeListMainWindow::GetPrevExpanded (const wxTreeItemId& item) const {
    wxCHECK_MSG (item.IsOk(), wxTreeItemId(), _T("invalid tree item"));
    return GetPrev (item, false);
}

wxTreeItemId wxTreeListMainWindow::GetFirstVisible(bool fullRow, bool within) const {
    if (HasFlag(wxTR_HIDE_ROOT) || ! IsVisible(GetRootItem(), fullRow, within)) {
        return GetNextVisible (GetRootItem(), fullRow, within);
    } else {
        return GetRootItem();
    }
}

wxTreeItemId wxTreeListMainWindow::GetNextVisible (const wxTreeItemId& item, bool fullRow, bool within) const {
    wxCHECK_MSG (item.IsOk(), wxTreeItemId(), _T("invalid tree item"));
    wxTreeItemId id = GetNext (item, false);
    while (id.IsOk()) {
        if (IsVisible (id, fullRow, within)) return id;
        id = GetNext (id, false);
    }
    return wxTreeItemId();
}

wxTreeItemId wxTreeListMainWindow::GetLastVisible ( bool fullRow, bool within) const {
    wxCHECK_MSG (GetRootItem().IsOk(), wxTreeItemId(), _T("invalid tree item"));
    wxTreeItemId id = GetRootItem();
    wxTreeItemId res = id;
    while ((id = GetNext (id, false)).IsOk()) {
        if (IsVisible (id, fullRow, within)) res = id;
    }
    return res;
}

wxTreeItemId wxTreeListMainWindow::GetPrevVisible (const wxTreeItemId& item, bool fullRow, bool within) const {
    wxCHECK_MSG (item.IsOk(), wxTreeItemId(), _T("invalid tree item"));
    wxTreeItemId id = GetPrev (item, true);
    while (id.IsOk()) {
        if (IsVisible (id, fullRow, within)) return id;
        id = GetPrev(id, true);
    }
    return wxTreeItemId();
}

// ----------------------------------------------------------------------------
// operations
// ----------------------------------------------------------------------------

// ----------------------------  ADD OPERATION  -------------------------------

wxTreeItemId wxTreeListMainWindow::DoInsertItem (const wxTreeItemId& parentId,
                                                 size_t previous,
                                                 const wxString& text,
                                                 int image, int selImage,
                                                 wxTreeItemData *data) {
    wxTreeListItem *parent = (wxTreeListItem*)parentId.m_pItem;
    wxCHECK_MSG (parent, wxTreeItemId(), _T("item must have a parent, at least root!") );
    m_dirty = true; // do this first so stuff below doesn't cause flicker

    wxArrayString arr;
    arr.Alloc (GetColumnCount());
    for (int i = 0; i < (int)GetColumnCount(); ++i) arr.Add (wxEmptyString);
    arr[m_main_column] = text;
    wxTreeListItem *item = new wxTreeListItem (this, parent, arr, image, selImage, data);
    if (data != NULL) {
        data->SetId (item);
    }
    parent->Insert (item, previous);

    return item;
}

wxTreeItemId wxTreeListMainWindow::AddRoot (const wxString& text,
                                            int image, int selImage,
                                            wxTreeItemData *data) {
    wxCHECK_MSG(!m_rootItem, wxTreeItemId(), _T("tree can have only one root"));
    wxCHECK_MSG(GetColumnCount(), wxTreeItemId(), _T("Add column(s) before adding the root item"));
    m_dirty = true; // do this first so stuff below doesn't cause flicker

    wxArrayString arr;
    arr.Alloc (GetColumnCount());
    for (int i = 0; i < (int)GetColumnCount(); ++i) arr.Add (wxEmptyString);
    arr[m_main_column] = text;
    m_rootItem = new wxTreeListItem (this, (wxTreeListItem *)NULL, arr, image, selImage, data);
    if (data != NULL) {
        data->SetId(m_rootItem);
    }
    if (HasFlag(wxTR_HIDE_ROOT)) {
        // if we will hide the root, make sure children are visible
        m_rootItem->SetHasPlus();
        m_rootItem->Expand();
        wxTreeItemIdValue cookie = 0;
        SetCurrentItem(GetFirstChild(m_rootItem, cookie));
    }
    return m_rootItem;
}

wxTreeItemId wxTreeListMainWindow::PrependItem (const wxTreeItemId& parent,
                                                const wxString& text,
                                                int image, int selImage,
                                                wxTreeItemData *data) {
    return DoInsertItem (parent, 0u, text, image, selImage, data);
}

wxTreeItemId wxTreeListMainWindow::InsertItem (const wxTreeItemId& parentId,
                                               const wxTreeItemId& idPrevious,
                                               const wxString& text,
                                               int image, int selImage,
                                               wxTreeItemData *data) {
    wxTreeListItem *parent = (wxTreeListItem*)parentId.m_pItem;
    wxCHECK_MSG (parent, wxTreeItemId(), _T("item must have a parent, at least root!") );

    int index = parent->GetChildren().Index((wxTreeListItem*) idPrevious.m_pItem);
    wxASSERT_MSG( index != wxNOT_FOUND,
                  _T("previous item in wxTreeListMainWindow::InsertItem() is not a sibling") );

    return DoInsertItem (parentId, ++index, text, image, selImage, data);
}

wxTreeItemId wxTreeListMainWindow::InsertItem (const wxTreeItemId& parentId,
                                               size_t before,
                                               const wxString& text,
                                               int image, int selImage,
                                               wxTreeItemData *data) {
    wxTreeListItem *parent = (wxTreeListItem*)parentId.m_pItem;
    wxCHECK_MSG (parent, wxTreeItemId(), _T("item must have a parent, at least root!") );

    return DoInsertItem (parentId, before, text, image, selImage, data);
}

wxTreeItemId wxTreeListMainWindow::AppendItem (const wxTreeItemId& parentId,
                                               const wxString& text,
                                               int image, int selImage,
                                               wxTreeItemData *data) {
    wxTreeListItem *parent = (wxTreeListItem*) parentId.m_pItem;
    wxCHECK_MSG (parent, wxTreeItemId(), _T("item must have a parent, at least root!") );

    return DoInsertItem (parent, parent->GetChildren().Count(), text, image, selImage, data);
}


// --------------------------  DELETE OPERATION  ------------------------------

void wxTreeListMainWindow::Delete (const wxTreeItemId& itemId) {
    if (! itemId.IsOk()) return;
    wxTreeListItem *item = (wxTreeListItem*) itemId.m_pItem;
    wxTreeListItem *parent = item->GetItemParent();
    wxCHECK_RET (item != m_rootItem, _T("invalid item, root may not be deleted this way!"));

    // recursive delete
    DoDeleteItem(item);

    // update parent --CAUTION: must come after delete itself, so that item's
    //  siblings may be found
    if (parent) {
        parent->GetChildren().Remove (item);  // remove by value
    }
}


void wxTreeListMainWindow::DeleteRoot() {
    if (! m_rootItem) return;

    SetCurrentItem((wxTreeListItem*)NULL);
    m_selectItem = (wxTreeListItem*)NULL;
    m_shiftItem = (wxTreeListItem*)NULL;

    DeleteChildren (m_rootItem);
    SendEvent(wxEVT_COMMAND_TREE_DELETE_ITEM, m_rootItem);
    delete m_rootItem; m_rootItem = NULL;
}


void wxTreeListMainWindow::DeleteChildren (const wxTreeItemId& itemId) {
    if (! itemId.IsOk()) return;
    wxTreeListItem *item = (wxTreeListItem*) itemId.m_pItem;

    // recursive delete on all children, starting from the right to prevent
    //  multiple selection changes (see m_curItem handling in DoDeleteItem() )
    wxArrayTreeListItems& children = item->GetChildren();
    for (size_t n = children.GetCount(); n>0; n--) {
        DoDeleteItem(children[n-1]);
        // immediately remove child from array, otherwise it might get selected
        // as current item (see m_curItem handling in DoDeleteItem() )
        children.RemoveAt(n-1);
    }
}


void wxTreeListMainWindow::DoDeleteItem(wxTreeListItem *item) {
    wxCHECK_RET (item, _T("invalid item for delete!"));

    m_dirty = true; // do this first so stuff below doesn't cause flicker

    // cancel any editing

    if (m_editControl) { m_editControl->EndEdit(true); }  // cancelled

    // cancel any dragging
    if (item == m_dragItem) {
        // stop dragging
        m_isDragStarted = m_isDragging = false;
        if (HasCapture()) ReleaseMouse();
    }

    // don't stay with invalid m_curItem: take next sibling or reset to NULL
    // NOTE: this might be slighty inefficient when deleting a whole tree
    //  but has the advantage that all deletion side-effects are handled here
    if (item == m_curItem) {
        SetCurrentItem(item->GetItemParent());
        if (m_curItem) {
            wxArrayTreeListItems& siblings = m_curItem->GetChildren();
            size_t index = siblings.Index (item);
            wxASSERT (index != (size_t)wxNOT_FOUND); // I'm not a child of my parent?
            SetCurrentItem(index < siblings.Count()-1 ? siblings[index+1]: (wxTreeListItem*)NULL);
        }
    }
    // don't stay with invalid m_shiftItem: reset it to NULL
    if (item == m_shiftItem) m_shiftItem = (wxTreeListItem*)NULL;
    // don't stay with invalid m_selectItem: default to current item
    if (item == m_selectItem) {
        m_selectItem = m_curItem;
        SelectItem(m_selectItem, (wxTreeItemId*)NULL, true);  // unselect others
    }

    // recurse children, starting from the right to prevent multiple selection
    //  changes (see m_curItem handling above)
    wxArrayTreeListItems& children = item->GetChildren();
    for (size_t n = children.GetCount(); n>0; n--) {
        DoDeleteItem(children[n-1]);
        // immediately remove child from array, otherwise it might get selected
        // as current item (see m_curItem handling above)
        children.RemoveAt(n-1);
    }

    // delete item itself
    wxTreeItemData* data = GetItemData(item);
    if (data != NULL)
    {
        delete data;
    };
    SendEvent(wxEVT_COMMAND_TREE_DELETE_ITEM, item);
    delete item;
}


// ----------------------------------------------------------------------------

void wxTreeListMainWindow::SetItemParent(const wxTreeItemId& parentId, const wxTreeItemId& itemId) {
wxTreeListItem *item = (wxTreeListItem*) itemId.m_pItem;
wxTreeListItem *parent_new = (wxTreeListItem*) parentId.m_pItem;
wxCHECK_RET (item, _T("invalid item in wxTreeListMainWindow::SetItemParent") );
wxCHECK_RET (parent_new, _T("invalid parent in wxTreeListMainWindow::SetItemParent") );
wxCHECK_RET (item != m_rootItem, _T("invalid root as item in wxTreeListMainWindow::SetItemParent!") );
wxTreeListItem *parent_old = item->GetItemParent();

    m_dirty = true; // do this first so stuff below doesn't cause flicker

    parent_old->GetChildren().Remove (item);
    parent_new->Insert(item, parent_new->GetChildren().Count());
    item->SetItemParent(parent_new);
    // new parent was a leaf, show its new child
    if (parent_new->GetChildren().Count() == 1) parent_new->Expand();
}


// ----------------------------------------------------------------------------

void wxTreeListMainWindow::SetCurrentItem(const wxTreeItemId& itemId) {
  SetCurrentItem((wxTreeListItem *)(itemId ? itemId.m_pItem : NULL));
}
void wxTreeListMainWindow::SetCurrentItem(wxTreeListItem *item) {
wxTreeListItem *old_item;

    old_item = m_curItem; m_curItem = item;

    // change of item, redraw previous
    if (old_item != NULL && old_item != item) {
        RefreshLine(old_item);
    }

}

// ----------------------------------------------------------------------------

void wxTreeListMainWindow::Expand (const wxTreeItemId& itemId) {
    wxTreeListItem *item = (wxTreeListItem*) itemId.m_pItem;
    wxCHECK_RET (item, _T("invalid item in wxTreeListMainWindow::Expand") );

    if (!item->HasPlus() || item->IsExpanded()) return;

    // send event to user code
    wxTreeEvent event(wxEVT_COMMAND_TREE_ITEM_EXPANDING, 0);
    event.SetInt(m_curColumn);
    if (SendEvent(0, item, &event) && !event.IsAllowed()) return; // expand canceled

    item->Expand();
    m_dirty = true;

    // send event to user code
    event.SetEventType (wxEVT_COMMAND_TREE_ITEM_EXPANDED);
    SendEvent(0, NULL, &event);
}

void wxTreeListMainWindow::ExpandAll (const wxTreeItemId& itemId) {
    wxCHECK_RET (itemId.IsOk(), _T("invalid tree item"));

    Expand (itemId);
    if (!IsExpanded (itemId)) return;
    wxTreeItemIdValue cookie;
    wxTreeItemId child = GetFirstChild (itemId, cookie);
    while (child.IsOk()) {
        ExpandAll (child);
        child = GetNextChild (itemId, cookie);
    }
}

void wxTreeListMainWindow::Collapse (const wxTreeItemId& itemId) {
    wxTreeListItem *item = (wxTreeListItem*) itemId.m_pItem;
    wxCHECK_RET (item, _T("invalid item in wxTreeListMainWindow::Collapse") );

    if (!item->HasPlus() || !item->IsExpanded()) return;

    // send event to user code
    wxTreeEvent event (wxEVT_COMMAND_TREE_ITEM_COLLAPSING, 0 );
    event.SetInt(m_curColumn);
    if (SendEvent(0, item, &event) && !event.IsAllowed()) return; // collapse canceled

    item->Collapse();
    m_dirty = true;

    // send event to user code
    event.SetEventType (wxEVT_COMMAND_TREE_ITEM_COLLAPSED);
    SendEvent(0, NULL, &event);
}

void wxTreeListMainWindow::CollapseAndReset (const wxTreeItemId& item) {
    wxCHECK_RET (item.IsOk(), _T("invalid tree item"));

    Collapse (item);
    DeleteChildren (item);
}

void wxTreeListMainWindow::Toggle (const wxTreeItemId& itemId) {
    wxCHECK_RET (itemId.IsOk(), _T("invalid tree item"));

    if (IsExpanded (itemId)) {
        Collapse (itemId);
    }else{
        Expand (itemId);
    }
}

void wxTreeListMainWindow::Unselect() {
    if (m_selectItem) {
        m_selectItem->SetHilight (false);
        RefreshLine (m_selectItem);
        m_selectItem = (wxTreeListItem*)NULL;
    }
}

void wxTreeListMainWindow::UnselectAllChildren (wxTreeListItem *item) {
    wxCHECK_RET (item, _T("invalid tree item"));

    if (item->IsSelected()) {
        item->SetHilight (false);
        RefreshLine (item);
        if (item == m_selectItem) m_selectItem = (wxTreeListItem*)NULL;
        if (item != m_curItem) m_lastOnSame = false;  // selection change, so reset edit marker
    }
    if (item->HasChildren()) {
        wxArrayTreeListItems& children = item->GetChildren();
        size_t count = children.Count();
        for (size_t n = 0; n < count; ++n) {
            UnselectAllChildren (children[n]);
        }
    }
}

void wxTreeListMainWindow::UnselectAll() {
    UnselectAllChildren ((wxTreeListItem*)GetRootItem().m_pItem);
}

// Recursive function !
// To stop we must have crt_item<last_item
// Algorithm :
// Tag all next children, when no more children,
// Move to parent (not to tag)
// Keep going... if we found last_item, we stop.
bool wxTreeListMainWindow::TagNextChildren (wxTreeListItem *crt_item,
                                            wxTreeListItem *last_item) {
    wxTreeListItem *parent = crt_item->GetItemParent();

    if (!parent) {// This is root item
        return TagAllChildrenUntilLast (crt_item, last_item);
    }

    wxArrayTreeListItems& children = parent->GetChildren();
    int index = children.Index(crt_item);
    wxASSERT (index != wxNOT_FOUND); // I'm not a child of my parent?

    if ((parent->HasChildren() && parent->IsExpanded()) ||
        ((parent == (wxTreeListItem*)GetRootItem().m_pItem) && HasFlag(wxTR_HIDE_ROOT))) {
        size_t count = children.Count();
        for (size_t n = (index+1); n < count; ++n) {
            if (TagAllChildrenUntilLast (children[n], last_item)) return true;
        }
    }

    return TagNextChildren (parent, last_item);
}

bool wxTreeListMainWindow::TagAllChildrenUntilLast (wxTreeListItem *crt_item,
                                                    wxTreeListItem *last_item) {
    crt_item->SetHilight (true);
    RefreshLine(crt_item);

    if (crt_item==last_item) return true;

    if (crt_item->HasChildren() && crt_item->IsExpanded()) {
        wxArrayTreeListItems& children = crt_item->GetChildren();
        size_t count = children.Count();
        for (size_t n = 0; n < count; ++n) {
            if (TagAllChildrenUntilLast (children[n], last_item)) return true;
        }
    }

    return false;
}

bool wxTreeListMainWindow::SelectItem (const wxTreeItemId& itemId,
                                       const wxTreeItemId& lastId,
                                       bool unselect_others) {

    wxTreeListItem *item = itemId.IsOk() ? (wxTreeListItem*) itemId.m_pItem : NULL;

    // send selecting event to the user code
    wxTreeEvent event( wxEVT_COMMAND_TREE_SEL_CHANGING, 0);
    event.SetInt(m_curColumn);
    event.SetOldItem (m_curItem);
    if (SendEvent(0, item, &event) && !event.IsAllowed()) return false;  // veto on selection change

    // unselect all if unselect other items
    bool bUnselectedAll = false; // see that UnselectAll is done only once
    if (unselect_others) {
        if (HasFlag(wxTR_MULTIPLE)) {
            UnselectAll(); bUnselectedAll = true;
        }else{
            Unselect(); // to speed up thing
        }
    }

    // select item range
    if (lastId.IsOk() && itemId.IsOk() && (itemId != lastId)) {

        if (! bUnselectedAll) UnselectAll();
        wxTreeListItem *last = (wxTreeListItem*) lastId.m_pItem;

        // ensure that the position of the item it calculated in any case
        if (m_dirty) CalculatePositions();

        // select item range according Y-position
        if (last->GetY() < item->GetY()) {
            if (!TagAllChildrenUntilLast (last, item)) {
                TagNextChildren (last, item);
            }
        }else{
            if (!TagAllChildrenUntilLast (item, last)) {
                TagNextChildren (item, last);
            }
        }

    // or select single item
    }else if (itemId.IsOk()) {

        // select item according its old selection
        item->SetHilight (!item->IsSelected());
        RefreshLine (item);
        if (unselect_others) {
            m_selectItem = (item->IsSelected())? item: (wxTreeListItem*)NULL;
        }

    // or select nothing
    } else {
        if (! bUnselectedAll) UnselectAll();
    }

    // send event to user code
    event.SetEventType(wxEVT_COMMAND_TREE_SEL_CHANGED);
    SendEvent(0, NULL, &event);

    return true;
}

void wxTreeListMainWindow::SelectAll() {
    wxTreeItemId root = GetRootItem();
    wxCHECK_RET (HasFlag(wxTR_MULTIPLE), _T("invalid tree style"));
    wxCHECK_RET (root.IsOk(), _T("no tree"));

    // send event to user code
    wxTreeEvent event (wxEVT_COMMAND_TREE_SEL_CHANGING, 0);
    event.SetOldItem (m_curItem);
    event.SetInt (-1); // no colum clicked
    if (SendEvent(0, m_rootItem, &event) && !event.IsAllowed()) return;  // selection change vetoed

    wxTreeItemIdValue cookie = 0;
    wxTreeListItem *first = (wxTreeListItem *)GetFirstChild (root, cookie).m_pItem;
    wxTreeListItem *last = (wxTreeListItem *)GetLastChild (root, cookie).m_pItem;
    if (!TagAllChildrenUntilLast (first, last)) {
        TagNextChildren (first, last);
    }

    // send event to user code
    event.SetEventType (wxEVT_COMMAND_TREE_SEL_CHANGED);
    SendEvent(0, NULL, &event);
}

void wxTreeListMainWindow::FillArray (wxTreeListItem *item,
                                      wxArrayTreeItemIds &array) const {
    if (item->IsSelected()) array.Add (wxTreeItemId(item));

    if (item->HasChildren()) {
        wxArrayTreeListItems& children = item->GetChildren();
        size_t count = children.GetCount();
        for (size_t n = 0; n < count; ++n) FillArray (children[n], array);
    }
}

size_t wxTreeListMainWindow::GetSelections (wxArrayTreeItemIds &array) const {
    array.Empty();
    wxTreeItemId idRoot = GetRootItem();
    if (idRoot.IsOk()) FillArray ((wxTreeListItem*) idRoot.m_pItem, array);
    return array.Count();
}

void wxTreeListMainWindow::EnsureVisible (const wxTreeItemId& item) {
    if (!item.IsOk()) return; // do nothing if no item

    // first expand all parent branches
    wxTreeListItem *gitem = (wxTreeListItem*) item.m_pItem;
    wxTreeListItem *parent = gitem->GetItemParent();
    while (parent) {
        Expand (parent);
        parent = parent->GetItemParent();
    }

    ScrollTo (item);
    RefreshLine (gitem);
}

void wxTreeListMainWindow::ScrollTo (const wxTreeItemId &item) {
    if (!item.IsOk()) return; // do nothing if no item

    // ensure that the position of the item it calculated in any case
    if (m_dirty) CalculatePositions();

    wxTreeListItem *gitem = (wxTreeListItem*) item.m_pItem;

    // now scroll to the item
    int item_y = gitem->GetY();

    int xUnit, yUnit;
    GetScrollPixelsPerUnit (&xUnit, &yUnit);
    int start_x = 0;
    int start_y = 0;
    GetViewStart (&start_x, &start_y);
    start_y *= yUnit;

    int client_h = 0;
    int client_w = 0;
    GetClientSize (&client_w, &client_h);

    int x = 0;
    int y = 0;
    m_rootItem->GetSize (x, y, this);
    x = m_owner->GetHeaderWindow()->GetWidth();
    y += yUnit + 2; // one more scrollbar unit + 2 pixels
    int x_pos = GetScrollPos( wxHORIZONTAL );

    if (item_y < start_y+3) {
        // going down, item should appear at top
        SetScrollbars (xUnit, yUnit, xUnit ? x/xUnit : 0, yUnit ? y/yUnit : 0, x_pos, yUnit ? item_y/yUnit : 0);
    }else if (item_y+GetLineHeight(gitem) > start_y+client_h) {
        // going up, item should appear at bottom
        item_y += yUnit + 2;
        SetScrollbars (xUnit, yUnit, xUnit ? x/xUnit : 0, yUnit ? y/yUnit : 0, x_pos, yUnit ? (item_y+GetLineHeight(gitem)-client_h)/yUnit : 0 );
    }
}

// TODO: tree sorting functions are not reentrant and not MT-safe!
static wxTreeListMainWindow *s_treeBeingSorted = NULL;

static int LINKAGEMODE tree_ctrl_compare_func(wxTreeListItem **item1, wxTreeListItem **item2)
{
    wxCHECK_MSG (s_treeBeingSorted, 0, _T("bug in wxTreeListMainWindow::SortChildren()") );
    return s_treeBeingSorted->OnCompareItems(*item1, *item2);
}

int wxTreeListMainWindow::OnCompareItems(const wxTreeItemId& item1, const wxTreeItemId& item2)
{
    return (m_sortColumn == -1
        ? m_owner->OnCompareItems (item1, item2)
        : (m_ReverseSortOrder
            ? m_owner->OnCompareItems (item2, item1, m_sortColumn)
            : m_owner->OnCompareItems (item1, item2, m_sortColumn)
        )
    );
}

void wxTreeListMainWindow::SortChildren (const wxTreeItemId& itemId, int column, bool reverseOrder) {
    wxCHECK_RET (itemId.IsOk(), _T("invalid tree item"));

    wxTreeListItem *item = (wxTreeListItem*) itemId.m_pItem;

    wxCHECK_RET (!s_treeBeingSorted,
                 _T("wxTreeListMainWindow::SortChildren is not reentrant") );

    wxArrayTreeListItems& children = item->GetChildren();
    if ( children.Count() > 1 ) {
        m_dirty = true;
        s_treeBeingSorted = this;
        m_sortColumn = column;  // -1 indicates legacy mode
        m_ReverseSortOrder = reverseOrder;
        children.Sort(tree_ctrl_compare_func);
        s_treeBeingSorted = NULL;
    }
}

bool wxTreeListMainWindow::MatchItemText(const wxString &itemText, const wxString &pattern, int mode) {
wxString searchText;

   if (mode & wxTL_MODE_FIND_PARTIAL) {
       searchText = itemText.Mid (0, pattern.Length());
   }else{
       searchText = itemText;
   }
   if (mode & wxTL_MODE_FIND_NOCASE) {
       if (searchText.CmpNoCase (pattern) == 0) return true;
   }else{
       if (searchText.Cmp (pattern) == 0) return true;
   }

   return false;
}


wxTreeItemId wxTreeListMainWindow::FindItem (const wxTreeItemId& item, int column, const wxString& str, int mode) {
    wxTreeItemIdValue cookie = 0;
    wxTreeItemId next = item;

    // start checking the next items
    wxString itemText;
    int col, col_start, col_end;
    if (column >= 0) { col_start = col_end = column; }
    else { col_start = 0; col_end = GetColumnCount() - 1; }

    // navigate tree
    while (true) {
        // go to next item
        if (next.IsOk()) {
            if (mode & wxTL_MODE_NAV_LEVEL) {
                next = GetNextSibling (next);
            }else if (mode & wxTL_MODE_NAV_VISIBLE) {
                next = GetNextVisible (next, false, true);
            }else if (mode & wxTL_MODE_NAV_EXPANDED) {
                next = GetNextExpanded (next);
            }else{ // (mode & wxTL_MODE_NAV_FULLTREE) default
                next = GetNext (next, true);
            }
        // not a valid item, start at the top of the tree
        } else {
            next = GetRootItem();
            if (next.IsOk() && HasFlag(wxTR_HIDE_ROOT)) {
                next = GetFirstChild (GetRootItem(), cookie);
            }
        }
        // end of tree (or back to beginning) ?
        if (! next.IsOk() || next == item) return (wxTreeItemId*)NULL;
        // check for a match
        for (col=col_start; col<=col_end; col++) {
            if (MatchItemText(GetItemText (next, col),str, mode)) return next;
        }
    }
    // should never get here
    return (wxTreeItemId*)NULL;
}

void wxTreeListMainWindow::SetDragItem (const wxTreeItemId& item) {
    wxTreeListItem *prevItem = m_dragItem;
    m_dragItem = (wxTreeListItem*) item.m_pItem;
    if (prevItem) RefreshLine (prevItem);
    if (m_dragItem) RefreshLine (m_dragItem);
}

void wxTreeListMainWindow::CalculateLineHeight() {
    wxClientDC dc (this);
    dc.SetFont (m_normalFont);
    m_lineHeight = (int)(dc.GetCharHeight() + m_linespacing);

    if (m_imageListNormal) {
        // Calculate a m_lineHeight value from the normal Image sizes.
        // May be toggle off. Then wxTreeListMainWindow will spread when
        // necessary (which might look ugly).
        int n = m_imageListNormal->GetImageCount();
        for (int i = 0; i < n ; i++) {
            int width = 0, height = 0;
            m_imageListNormal->GetSize(i, width, height);
            if (height > m_lineHeight) m_lineHeight = height + m_linespacing;
        }
    }

    if (m_imageListButtons) {
        // Calculate a m_lineHeight value from the Button image sizes.
        // May be toggle off. Then wxTreeListMainWindow will spread when
        // necessary (which might look ugly).
        int n = m_imageListButtons->GetImageCount();
        for (int i = 0; i < n ; i++) {
            int width = 0, height = 0;
            m_imageListButtons->GetSize(i, width, height);
            if (height > m_lineHeight) m_lineHeight = height + m_linespacing;
        }
    }

    if (m_lineHeight < 30) { // add 10% space if greater than 30 pixels
        m_lineHeight += 2; // minimal 2 pixel space
    }else{
        m_lineHeight += m_lineHeight / 10; // otherwise 10% space
    }
}

void wxTreeListMainWindow::SetImageList (wxImageList *imageList) {
    if (m_ownsImageListNormal) delete m_imageListNormal;
    m_imageListNormal = imageList;
    m_ownsImageListNormal = false;
    m_dirty = true;
    CalculateLineHeight();
}

void wxTreeListMainWindow::SetStateImageList (wxImageList *imageList) {
    if (m_ownsImageListState) delete m_imageListState;
    m_imageListState = imageList;
    m_ownsImageListState = false;
}

void wxTreeListMainWindow::SetButtonsImageList (wxImageList *imageList) {
    if (m_ownsImageListButtons) delete m_imageListButtons;
    m_imageListButtons = imageList;
    m_ownsImageListButtons = false;
    m_dirty = true;
    CalculateLineHeight();
}

void wxTreeListMainWindow::AssignImageList (wxImageList *imageList) {
    SetImageList(imageList);
    m_ownsImageListNormal = true;
}

void wxTreeListMainWindow::AssignStateImageList (wxImageList *imageList) {
    SetStateImageList(imageList);
    m_ownsImageListState = true;
}

void wxTreeListMainWindow::AssignButtonsImageList (wxImageList *imageList) {
    SetButtonsImageList(imageList);
    m_ownsImageListButtons = true;
}

// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

void wxTreeListMainWindow::AdjustMyScrollbars() {
    if (m_rootItem) {
        int xUnit, yUnit;
        GetScrollPixelsPerUnit (&xUnit, &yUnit);
        if (xUnit == 0) xUnit = GetCharWidth();
        if (yUnit == 0) yUnit = m_lineHeight;
        int x = 0, y = 0;
        m_rootItem->GetSize (x, y, this);
        y += yUnit + 2; // one more scrollbar unit + 2 pixels
        int x_pos = GetScrollPos (wxHORIZONTAL);
        int y_pos = GetScrollPos (wxVERTICAL);
        x = m_owner->GetHeaderWindow()->GetWidth() + 2;
        if (x < GetClientSize().GetWidth()) x_pos = 0;
        SetScrollbars (xUnit, yUnit, x/xUnit, y/yUnit, x_pos, y_pos);
    }else{
        SetScrollbars (0, 0, 0, 0);
    }
}

int wxTreeListMainWindow::GetLineHeight (wxTreeListItem *item) const {
    if (GetWindowStyleFlag() & wxTR_HAS_VARIABLE_ROW_HEIGHT) {
        return item->GetHeight();
    }else{
        return m_lineHeight;
    }
}

void wxTreeListMainWindow::PaintItem (wxTreeListItem *item, wxDC& dc) {

// read attributes constant for all item cells
    wxColour colText = GetItemTextColour(item);
    wxColour colBg = GetItemBackgroundColour(item);
    wxColour colTextHilight = wxSystemSettings::GetColour (wxSYS_COLOUR_HIGHLIGHTTEXT);
    int total_w = std::max(m_owner->GetHeaderWindow()->GetWidth(), m_owner->GetMainWindow()->GetClientSize().GetWidth());
    int total_h = GetLineHeight(item);
    int off_h = HasFlag(wxTR_ROW_LINES) ? 1 : 0;
    int off_w = HasFlag(wxTR_COLUMN_LINES) ? 1 : 0;
    wxDCClipper clipper (dc, 0, item->GetY(), total_w, total_h); // only within line
    // compute text height based on main col
    int text_h = 0;
    dc.GetTextExtent( !item->GetText(GetMainColumn()).empty()
            ? item->GetText(GetMainColumn())
            : _T("M"),  // dummy text to avoid zero height and no highlight width
        NULL, &text_h );

// determine background and show it
// in wxTR_FULL_ROW_HIGHLIGHT mode, some drawing can be done already now
    dc.SetBrush (wxBrush ( colBg, wxBRUSHSTYLE_SOLID));
    dc.SetPen (*wxTRANSPARENT_PEN);
    if (HasFlag (wxTR_FULL_ROW_HIGHLIGHT)) {
        if (item->IsSelected()) {
            if (! m_isDragging && m_hasFocus) {
                dc.SetBrush (*m_hilightBrush);
#ifndef __WXMAC__ // don't draw rect outline if we already have the background color
                dc.SetPen (*wxBLACK_PEN);
#endif // !__WXMAC__
            }else{
                dc.SetBrush (*m_hilightUnfocusedBrush);
#ifndef __WXMAC__ // don't draw rect outline if we already have the background color
                dc.SetPen (*wxTRANSPARENT_PEN);
#endif // !__WXMAC__
            }
            dc.SetTextForeground (colTextHilight);
        }else {
            dc.SetTextForeground (GetItemTextColour(item));
            if (item == m_curItem) {
                dc.SetPen (m_hasFocus? *wxBLACK_PEN: *wxTRANSPARENT_PEN);
            }
        }
        dc.DrawRectangle (0, item->GetY() + off_h, total_w, total_h - off_h);
    }

// iterate through all cells
    int text_extraH = (total_h > text_h) ? (total_h - text_h)/2 : 0;
    int img_extraH = (total_h > m_imgHeight)? (total_h-m_imgHeight)/2: 0;
    int x_colstart = 0;
    for (int i = 0; i < GetColumnCount(); ++i ) {
        if (!m_owner->GetHeaderWindow()->IsColumnShown(i)) continue;
        int col_w = m_owner->GetHeaderWindow()->GetColumnWidth(i);
        if (col_w <= 0) continue;  // workaround for probable GTK2 bug [wxCode-Bugs-#3061215]
        wxDCClipper clipper (dc, x_colstart, item->GetY(), col_w, total_h); // only within column

        // read variable attributes
        dc.SetFont (GetItemFont (item, i));
        colText = GetItemTextColour(item, i);
        colBg = GetItemBackgroundColour(item, i);

        //
        int x = 0;
        int image = NO_IMAGE;
        int image_w = 0;
        if(i == GetMainColumn()) {
            x = item->GetX() + MARGIN;
            if (HasButtons()) {
                x += (m_btnWidth-m_btnWidth2) + LINEATROOT;
            }else{
                x -= m_indent/2;
            }
            if (m_imageListNormal) image = item->GetCurrentImage();
        }else{
            x = x_colstart + MARGIN;
            image = item->GetImage(i);
        }
        if (image != NO_IMAGE) image_w = m_imgWidth + MARGIN;

        // honor text alignment
        int w = 0, text_w = 0;
        wxString text = item->GetText(i);
        dc.GetTextExtent (text, &text_w, NULL);
        switch ( m_owner->GetHeaderWindow()->GetColumn(i).GetAlignment() ) {
        case wxALIGN_LEFT:
            // nothing to do, already left aligned
            break;
        case wxALIGN_RIGHT:
            w = col_w - (image_w + text_w + off_w + MARGIN);
            x += (w > 0)? w: 0;
            break;
        case wxALIGN_CENTER:
            w = (col_w - (image_w + text_w + off_w + MARGIN))/2;
            x += (w > 0)? w: 0;
            break;
        }
        int text_x = x + image_w;
        item->SetTextX (i, text_x);

        // draw background (in non wxTR_FULL_ROW_HIGHLIGHT mode)
        // cell-specific settings are used --excepted for selection:
        if ( ! HasFlag (wxTR_FULL_ROW_HIGHLIGHT)) {
            // cursor: indicate current cell
            bool drawCursor = false;
#ifndef __WXMAC__ // don't draw rect outline if we already have the background color
            drawCursor = (item == m_curItem && i == m_curColumn && !m_isDragging && m_hasFocus);
#endif // !__WXMAC__
            // selection: main col only, overrides colors + separate draw
            if (item->IsSelected() && i == GetMainColumn()) {
                // draw normal background
                dc.SetPen (*wxTRANSPARENT_PEN);
                dc.SetBrush (wxBrush ( colBg, wxBRUSHSTYLE_SOLID));
                dc.DrawRectangle (x_colstart, item->GetY() + off_h, col_w, total_h - off_h);
                // draw selection & optionally cursor
                dc.SetPen (drawCursor ? *wxBLACK_PEN : *wxTRANSPARENT_PEN);
                dc.SetBrush(!m_isDragging && m_hasFocus ? *m_hilightBrush : *m_hilightUnfocusedBrush);
                dc.SetTextForeground (colTextHilight);
                dc.DrawRectangle (text_x, item->GetY() + off_h, text_w, total_h - off_h);
            // normal FG / BG from attributes
            } else {
                // draw normal background & optionally cursor
                dc.SetPen (drawCursor && i != GetMainColumn() ? *wxBLACK_PEN : *wxTRANSPARENT_PEN);
                dc.SetBrush (wxBrush ( colBg, wxBRUSHSTYLE_SOLID));
                dc.SetTextForeground (colText);
                dc.DrawRectangle (x_colstart, item->GetY() + off_h, col_w, total_h - off_h);
                // on main col draw a separate cursor
                if (drawCursor && i == GetMainColumn()) {
                    dc.SetPen (*wxBLACK_PEN);
                    dc.SetBackgroundMode (wxTRANSPARENT);
                    dc.DrawRectangle (text_x, item->GetY() + off_h, text_w, total_h - off_h);
                }
            }
        }

        // draw vertical column lines
        if (HasFlag(wxTR_COLUMN_LINES)) { // vertical lines between columns
            wxPen pen (wxSystemSettings::GetColour (wxSYS_COLOUR_3DLIGHT ), 1, wxPENSTYLE_SOLID);
            dc.SetPen ((GetBackgroundColour() == *wxWHITE)? pen: *wxWHITE_PEN);
            dc.DrawLine (x_colstart+col_w-1, item->GetY(), x_colstart+col_w-1, item->GetY()+total_h);
        }

        dc.SetBackgroundMode (wxTRANSPARENT);

        // draw image
        if (image != NO_IMAGE && m_imageListNormal && image < m_imageListNormal->GetImageCount()) {
            int y = item->GetY() + img_extraH;
            m_imageListNormal->Draw (image, dc, x, y, wxIMAGELIST_DRAW_TRANSPARENT );
        }

        // draw text
        int text_y = item->GetY() + text_extraH;
        dc.DrawText (text, (wxCoord)text_x, (wxCoord)text_y);

        x_colstart += col_w;
    }

    // restore normal font
    dc.SetFont( m_normalFont );
}

// Now y stands for the top of the item, whereas it used to stand for middle !
void wxTreeListMainWindow::PaintLevel (wxTreeListItem *item, wxDC &dc,
                                       int level, int &y, int x_maincol) {

    // Handle hide root (only level 0)
    if (HasFlag(wxTR_HIDE_ROOT) && (level == 0)) {
        wxArrayTreeListItems& children = item->GetChildren();
        for (size_t n = 0; n < children.Count(); n++) {
            PaintLevel (children[n], dc, 1, y, x_maincol);
        }
        // end after expanding root
        return;
    }

    // calculate position of vertical lines
    int x = x_maincol + MARGIN; // start of column
    if (HasFlag(wxTR_LINES_AT_ROOT)) x += LINEATROOT; // space for lines at root
    if (HasButtons()) {
        x += (m_btnWidth-m_btnWidth2); // half button space
    }else{
        x += (m_indent-m_indent/2);
    }
    if (HasFlag(wxTR_HIDE_ROOT)) {
        x += m_indent * (level-1); // indent but not level 1
    }else{
        x += m_indent * level; // indent according to level
    }

    // set position of vertical line
    item->SetX (x);
    item->SetY (y);

    int h = GetLineHeight (item);
    int y_top = y;
    int y_mid = y_top + (h/2);
    y += h;

    int exposed_x = dc.LogicalToDeviceX(0);
    int exposed_y = dc.LogicalToDeviceY(y_top);

    if (IsExposed(exposed_x, exposed_y, 10000, h)) { // 10000 = very much

        if (HasFlag(wxTR_ROW_LINES)) { // horizontal lines between rows
            //dc.DestroyClippingRegion();
            int total_width = std::max(m_owner->GetHeaderWindow()->GetWidth(), m_owner->GetClientSize().GetWidth());
            dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_3DLIGHT), 1, wxPENSTYLE_SOLID));
            dc.DrawLine (0, y_top, total_width, y_top);
            dc.DrawLine (0, y_top+h, total_width, y_top+h);
        }

        // draw item
        PaintItem (item, dc);

        // restore DC objects
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.SetPen(m_dottedPen);

        // clip to the column width
        int clip_width = m_owner->GetHeaderWindow()->
                            GetColumn(m_main_column).GetWidth();
        wxDCClipper clipper(dc, x_maincol, y_top, clip_width, 10000);

        if (!HasFlag(wxTR_NO_LINES)) { // connection lines

            // draw the horizontal line here
            dc.SetPen(m_dottedPen);
            int x2 = x - m_indent;
            if (x2 < (x_maincol + MARGIN)) x2 = x_maincol + MARGIN;
            int x3 = x + (m_btnWidth-m_btnWidth2);
            if (HasButtons()) {
                if (item->HasPlus()) {
                    dc.DrawLine (x2, y_mid, x - m_btnWidth2, y_mid);
                    dc.DrawLine (x3, y_mid, x3 + LINEATROOT, y_mid);
                }else{
                    dc.DrawLine (x2, y_mid, x3 + LINEATROOT, y_mid);
                }
            }else{
                dc.DrawLine (x2, y_mid, x - m_indent/2, y_mid);
            }
        }

        if (item->HasPlus() && HasButtons()) { // should the item show a button?

            if (m_imageListButtons) {

                // draw the image button here
                int image = wxTreeItemIcon_Normal;
                if (item->IsExpanded()) image = wxTreeItemIcon_Expanded;
                if (item->IsSelected()) image += wxTreeItemIcon_Selected - wxTreeItemIcon_Normal;
                int xx = x - m_btnWidth2 + MARGIN;
                int yy = y_mid - m_btnHeight2;
                dc.SetClippingRegion(xx, yy, m_btnWidth, m_btnHeight);
                m_imageListButtons->Draw (image, dc, xx, yy, wxIMAGELIST_DRAW_TRANSPARENT);
                dc.DestroyClippingRegion();

            }else if (HasFlag (wxTR_TWIST_BUTTONS)) {

                // draw the twisty button here
                dc.SetPen(*wxBLACK_PEN);
                dc.SetBrush(*m_hilightBrush);
                wxPoint button[3];
                if (item->IsExpanded()) {
                    button[0].x = x - (m_btnWidth2+1);
                    button[0].y = y_mid - (m_btnHeight/3);
                    button[1].x = x + (m_btnWidth2+1);
                    button[1].y = button[0].y;
                    button[2].x = x;
                    button[2].y = button[0].y + (m_btnHeight2+1);
                }else{
                    button[0].x = x - (m_btnWidth/3);
                    button[0].y = y_mid - (m_btnHeight2+1);
                    button[1].x = button[0].x;
                    button[1].y = y_mid + (m_btnHeight2+1);
                    button[2].x = button[0].x + (m_btnWidth2+1);
                    button[2].y = y_mid;
                }
                dc.DrawPolygon(3, button);

            }else{ // if (HasFlag(wxTR_HAS_BUTTONS))

                // draw the plus sign here
                wxRect rect (x-m_btnWidth2, y_mid-m_btnHeight2, m_btnWidth, m_btnHeight);
                int flag = item->IsExpanded()? wxCONTROL_EXPANDED: 0;
                wxRendererNative::Get().DrawTreeItemButton (this, dc, rect, flag);
            }

        }

    }

    // restore DC objects
    dc.SetBrush(*wxWHITE_BRUSH);
    dc.SetPen(m_dottedPen);
    dc.SetTextForeground(*wxBLACK);

    if (item->IsExpanded())
    {
        wxArrayTreeListItems& children = item->GetChildren();

        // clip to the column width
        int clip_width = m_owner->GetHeaderWindow()->
                            GetColumn(m_main_column).GetWidth();

        // process lower levels
        int oldY;
        if (m_imgWidth > 0) {
            oldY = y_mid + m_imgHeight2;
        }else{
            oldY = y_mid + h/2;
        }
        int y2;
        for (size_t n = 0; n < children.Count(); ++n) {

            y2 = y + h/2;
            PaintLevel (children[n], dc, level+1, y, x_maincol);

            // draw vertical line
            wxDCClipper clipper(dc, x_maincol, y_top, clip_width, 10000);
            if (!HasFlag (wxTR_NO_LINES)) {
                x = item->GetX();
                dc.DrawLine (x, oldY, x, y2);
                oldY = y2;
            }
        }
    }
}


// ----------------------------------------------------------------------------
// wxWindows callbacks
// ----------------------------------------------------------------------------

void wxTreeListMainWindow::OnPaint (wxPaintEvent &WXUNUSED(event)) {

    // init device context, clear background (BEFORE changing DC origin...)
    wxAutoBufferedPaintDC dc (this);
    wxBrush brush(GetBackgroundColour(), wxBRUSHSTYLE_SOLID);
    dc.SetBackground(brush);
    dc.Clear();
    DoPrepareDC (dc);

    if (!m_rootItem || (GetColumnCount() <= 0)) return;

    // calculate button size
    if (m_imageListButtons) {
        m_imageListButtons->GetSize (0, m_btnWidth, m_btnHeight);
    }else if (HasButtons()) {
        m_btnWidth = BTNWIDTH;
        m_btnHeight = BTNHEIGHT;
    }
    m_btnWidth2 = m_btnWidth/2;
    m_btnHeight2 = m_btnHeight/2;

    // calculate image size
    if (m_imageListNormal) {
        m_imageListNormal->GetSize (0, m_imgWidth, m_imgHeight);
    }
    m_imgWidth2 = m_imgWidth/2;
    m_imgHeight2 = m_imgHeight/2;

    // calculate indent size
    if (m_imageListButtons) {
        m_indent = wxMax (MININDENT, m_btnWidth + MARGIN);
    }else if (HasButtons()) {
        m_indent = wxMax (MININDENT, m_btnWidth + LINEATROOT);
    }

    // set default values
    dc.SetFont( m_normalFont );
    dc.SetPen( m_dottedPen );

    // calculate column start and paint
    int x_maincol = 0;
    int i = 0;
    for (i = 0; i < (int)GetMainColumn(); ++i) {
        if (!m_owner->GetHeaderWindow()->IsColumnShown(i)) continue;
        x_maincol += m_owner->GetHeaderWindow()->GetColumnWidth (i);
    }
    int y = 0;
    PaintLevel (m_rootItem, dc, 0, y, x_maincol);
}

void wxTreeListMainWindow::OnSetFocus (wxFocusEvent &event) {
    m_hasFocus = true;
    RefreshSelected();
    if (m_curItem) RefreshLine (m_curItem);
    event.Skip();
}

void wxTreeListMainWindow::OnKillFocus( wxFocusEvent &event )
{
    m_hasFocus = false;
    RefreshSelected();
    if (m_curItem) RefreshLine (m_curItem);
    event.Skip();
}

void wxTreeListMainWindow::OnChar (wxKeyEvent &event) {
    // send event to user code
    wxTreeEvent nevent (wxEVT_COMMAND_TREE_KEY_DOWN, 0 );
    nevent.SetInt(m_curColumn);
    nevent.SetKeyEvent (event);
    // store modifiers in extra long for Mac
    nevent.SetExtraLong(event.GetModifiers());
    if (SendEvent(0, NULL, &nevent)) return; // char event handled in user code

    // if no item current, select root
    bool curItemSet = false;
    if (!m_curItem) {
        if (! GetRootItem().IsOk()) return;
        SetCurrentItem((wxTreeListItem*)GetRootItem().m_pItem);
        if (HasFlag(wxTR_HIDE_ROOT)) {
            wxTreeItemIdValue cookie = 0;
            SetCurrentItem((wxTreeListItem*)GetFirstChild (m_curItem, cookie).m_pItem);
        }
        SelectItem(m_curItem, (wxTreeItemId*)NULL, true);  // unselect others
        curItemSet = true;
    }

    // remember item at shift down
    if (HasFlag(wxTR_MULTIPLE) && event.ShiftDown()) {
        if (!m_shiftItem) m_shiftItem = m_curItem;
    }else{
        m_shiftItem = (wxTreeListItem*)NULL;
    }

    if (curItemSet) return;  // if no item was current until now, do nothing more

    // process all cases
    wxTreeItemId newItem = (wxTreeItemId*)NULL;
    switch (event.GetKeyCode()) {

        // '+': Expand subtree
        case '+':
        case WXK_ADD: {
            if (m_curItem->HasPlus() && !IsExpanded (m_curItem)) Expand (m_curItem);
        }break;

        // '-': collapse subtree
        case '-':
        case WXK_SUBTRACT: {
            if (m_curItem->HasPlus() && IsExpanded (m_curItem)) Collapse (m_curItem);
        }break;

        // '*': expand/collapse all subtrees // TODO: Mak it more useful
        case '*':
        case WXK_MULTIPLY: {
            if (m_curItem->HasPlus() && !IsExpanded (m_curItem)) {
                ExpandAll (m_curItem);
            }else if (m_curItem->HasPlus()) {
                Collapse (m_curItem); // TODO: CollapseAll
            }
        }break;

        // ' ': toggle current item
        case ' ': {
            SelectItem (m_curItem, (wxTreeListItem*)NULL, false);
        }break;

        // <RETURN>: activate current item
        case WXK_RETURN: {
            if (! SendEvent(wxEVT_COMMAND_TREE_ITEM_ACTIVATED, m_curItem)) {

                // if the user code didn't process the activate event,
                // handle it ourselves by toggling the item when it is
                // double clicked
                if (m_curItem && m_curItem->HasPlus()) Toggle(m_curItem);
            }
        }break;

        // <BKSP>: go to the parent without collapsing
        case WXK_BACK: {
            newItem = GetItemParent (m_curItem);
            if ((newItem == GetRootItem()) && HasFlag(wxTR_HIDE_ROOT)) {
                newItem = GetPrevSibling (m_curItem); // get sibling instead of root
            }
        }break;

        // <HOME>: go to first visible
        case WXK_HOME: {
            newItem = GetFirstVisible(false, false);
        }break;

        // <PAGE-UP>: go to the top of the page, or if we already are then one page back
        case WXK_PAGEUP: {
        int flags = 0;
        int col = 0;
        wxPoint abs_p = CalcUnscrolledPosition (wxPoint(1,1));
        // PAGE-UP: first go the the first visible row
            newItem = m_rootItem->HitTest(abs_p, this, flags, col, 0);
            newItem = GetFirstVisible(false, true);
        // if we are already there then scroll back one page
            if (newItem == m_curItem) {
                abs_p.y -= GetClientSize().GetHeight() - m_curItem->GetHeight();
                if (abs_p.y < 0) abs_p.y = 0;
                newItem = m_rootItem->HitTest(abs_p, this, flags, col, 0);
            }
            // newItem should never be NULL
        } break;

        // <UP>: go to the previous sibling or for the last of its children, to the parent
        case WXK_UP: {
            newItem = GetPrevSibling (m_curItem);
            if (newItem) {
                wxTreeItemIdValue cookie = 0;
                while (IsExpanded (newItem) && HasChildren (newItem)) {
                    newItem = GetLastChild (newItem, cookie);
                }
            }else {
                newItem = GetItemParent (m_curItem);
                if ((newItem == GetRootItem()) && HasFlag(wxTR_HIDE_ROOT)) {
                    newItem = (wxTreeItemId*)NULL; // don't go to root if it is hidden
                }
            }
        }break;

        // <LEFT>: if expanded collapse subtree, else go to the parent
        case WXK_LEFT: {
            if (IsExpanded (m_curItem)) {
                Collapse (m_curItem);
            }else{
                newItem = GetItemParent (m_curItem);
                if ((newItem == GetRootItem()) && HasFlag(wxTR_HIDE_ROOT)) {
                    newItem = GetPrevSibling (m_curItem); // go to sibling if it is hidden
                }
            }
        }break;

        // <RIGHT>: if possible expand subtree, else go go to the first child
        case WXK_RIGHT: {
            if (m_curItem->HasPlus() && !IsExpanded (m_curItem)) {
                Expand (m_curItem);
            }else{
                if (IsExpanded (m_curItem) && HasChildren (m_curItem)) {
                    wxTreeItemIdValue cookie = 0;
                    newItem = GetFirstChild (m_curItem, cookie);
                }
            }
        }break;

        // <DOWN>: if expanded go to the first child, else to the next sibling, ect
        case WXK_DOWN: {
            if (IsExpanded (m_curItem) && HasChildren (m_curItem)) {
                wxTreeItemIdValue cookie = 0;
                newItem = GetFirstChild( m_curItem, cookie );
            }
            if (!newItem) {
                wxTreeItemId parent = m_curItem;
                do {
                    newItem = GetNextSibling (parent);
                    parent = GetItemParent (parent);
                } while (!newItem && parent);
            }
        }break;

        // <PAGE-DOWN>: go to the bottom of the page, or if we already are then one page further
        case WXK_PAGEDOWN: {
        int flags = 0;
        int col = 0;
        wxPoint abs_p = CalcUnscrolledPosition (wxPoint(1,GetClientSize().GetHeight() - m_curItem->GetHeight()));
        // PAGE-UP: first go the the first visible row
            newItem = m_rootItem->HitTest(abs_p, this, flags, col, 0);
            newItem = GetLastVisible(false, true);
        // if we are already there then scroll down one page
            if (newItem == m_curItem) {
                abs_p.y += GetClientSize().GetHeight() - m_curItem->GetHeight();
//                if (abs_p.y >= GetVirtualSize().GetHeight()) abs_p.y = GetVirtualSize().GetHeight() - 1;
                newItem = m_rootItem->HitTest(abs_p, this, flags, col, 0);
            }
        // if we reached the empty area below the rows, return last item instead
            if (! newItem) newItem = GetLastVisible(false, false);
        } break;

        // <END>: go to last item of the root
        case WXK_END: {
            newItem = GetLastVisible (false, false);
        }break;

        // any char: go to the next matching string
        default:
            wxChar key = event.GetUnicodeKey();
            if (key == WXK_NONE) key = (wxChar)event.GetKeyCode();
            if (key  >= 32) {
                // prepare search parameters
                int mode = wxTL_MODE_NAV_EXPANDED | wxTL_MODE_FIND_PARTIAL | wxTL_MODE_FIND_NOCASE;
                if (!m_findTimer->IsRunning()) m_findStr.Clear();
                m_findStr.Append (key);
                m_findTimer->Start (FIND_TIMER_TICKS, wxTIMER_ONE_SHOT);
                wxTreeItemId prev = (wxTreeItemId*)NULL;
                // try if current item or one of its followers matches
                if (m_curItem) {
                    prev = (wxTreeItemId*)m_curItem;
                    for (int col=0; col<=GetColumnCount() - 1; col++) {
                        if (MatchItemText(GetItemText(prev, col), m_findStr, mode)) {
                            newItem = prev;
                            break;
                        }
                    }
                    if (! newItem) {
                        newItem = FindItem (prev, -1, m_findStr, mode);
                    };
                }
                // current item or does not match: try to find next
                // still not match: search from beginning (but only if there was a current item i.e.we did not start from root already)
                if (! newItem) {
                    prev = (wxTreeItemId*)NULL;
                    newItem = FindItem (prev, -1, m_findStr, mode);
                }
                // no match at all: remove just typed char to allow try with another extension
                if (! newItem) m_findStr.RemoveLast();
            }
            event.Skip();

    }

    // select and show the new item
    if (newItem) {
        if (!event.ControlDown()) {
            bool unselect_others = !((event.ShiftDown() || event.ControlDown()) &&
                                      HasFlag(wxTR_MULTIPLE));
            SelectItem (newItem, m_shiftItem, unselect_others);
        }
        EnsureVisible (newItem);
        wxTreeListItem *oldItem = m_curItem;
        SetCurrentItem((wxTreeListItem*)newItem.m_pItem); // make the new item the current item
        RefreshLine (oldItem);
    }

}

wxTreeItemId wxTreeListMainWindow::HitTest (const wxPoint& point, int& flags, int& column) {

    int w, h;
    GetSize(&w, &h);
    flags=0;
    column = -1;
    if (point.x<0) flags |= wxTREE_HITTEST_TOLEFT;
    if (point.x>w) flags |= wxTREE_HITTEST_TORIGHT;
    if (point.y<0) flags |= wxTREE_HITTEST_ABOVE;
    if (point.y>h) flags |= wxTREE_HITTEST_BELOW;
    if (flags) return wxTreeItemId();

    if (!m_rootItem) {
        flags = wxTREE_HITTEST_NOWHERE;
        column = -1;
        return wxTreeItemId();
    }

    wxTreeListItem *hit = m_rootItem->HitTest (CalcUnscrolledPosition(point),
                                               this, flags, column, 0);
    if (!hit) {
        flags = wxTREE_HITTEST_NOWHERE;
        column = -1;
        return wxTreeItemId();
    }
    return hit;
}

// get the bounding rectangle of the item (or of its label only)
bool wxTreeListMainWindow::GetBoundingRect (const wxTreeItemId& itemId, wxRect& rect,
                                            bool WXUNUSED(textOnly)) const {
    wxCHECK_MSG (itemId.IsOk(), false, _T("invalid item in wxTreeListMainWindow::GetBoundingRect") );

    wxTreeListItem *item = (wxTreeListItem*) itemId.m_pItem;

    int xUnit, yUnit;
    GetScrollPixelsPerUnit (&xUnit, &yUnit);
    int startX, startY;
    GetViewStart(& startX, & startY);

    rect.x = item->GetX() - startX * xUnit;
    rect.y = item->GetY() - startY * yUnit;
    rect.width = item->GetWidth();
    rect.height = GetLineHeight (item);

    return true;
}

/* **** */

void wxTreeListMainWindow::EditLabel (const wxTreeItemId& item, int column) {

// validate
    if (!item.IsOk()) return;
    if (!((column >= 0) && (column < GetColumnCount()))) return;

// cancel any editing
    if (m_editControl) { m_editControl->EndEdit(true); }  // cancelled

// prepare edit (position)
    m_editItem = (wxTreeListItem*) item.m_pItem;

    wxTreeEvent te( wxEVT_COMMAND_TREE_BEGIN_LABEL_EDIT, 0 );
    te.SetInt (column);
    SendEvent(0, m_editItem, &te); if (!te.IsAllowed()) return;

    // ensure that the position of the item it calculated in any case
    if (m_dirty) CalculatePositions();

    wxTreeListHeaderWindow* header_win = m_owner->GetHeaderWindow();

    // position & size are rather unpredictable (tsssk, tssssk) so were
    //  set by trial & error (on Win 2003 pre-XP style)
    int x = 0;
    int w = +4;  // +4 is necessary, don't know why (simple border erronously counted somewhere ?)
    int y = m_editItem->GetY() + 1;  // this is cell, not text
    int h = m_editItem->GetHeight() - 1;  // consequence from above
    long style = 0;
    if (column == GetMainColumn()) {
        x += m_editItem->GetTextX(column) - 2;  // wrong by 2, don't know why
        w += m_editItem->GetWidth();
    } else {
        for (int i = 0; i < column; ++i) {
            if ( header_win->IsColumnShown(i) ) {
                x += header_win->GetColumnWidth (i); // start of column
            }
		}
        w += header_win->GetColumnWidth (column);  // currently non-main column width not pre-computed
    }
    switch (header_win->GetColumnAlignment (column)) {
        case wxALIGN_LEFT:   {style = wxTE_LEFT;   x -= 1; break;}
        case wxALIGN_CENTER: {style = wxTE_CENTER; x -= 1; break;}
        case wxALIGN_RIGHT:  {style = wxTE_RIGHT;  x += 0; break;}  // yes, strange but that's the way it is
    }
    // wxTextCtrl simple border style requires 2 extra pixels before and after
    //  (measured by changing to style wxNO_BORDER in wxEditTextCtrl::wxEditTextCtrl() )
    y -= 2; x -= 2;
    w += 4; h += 4;

    wxClientDC dc (this);
    PrepareDC (dc);
    x = dc.LogicalToDeviceX (x);
    y = dc.LogicalToDeviceY (y);

// now do edit (change state, show control)
    m_editCol = column;  // only used in OnRenameAccept()
    m_editControl = new wxEditTextCtrl (this, -1, &m_editAccept, &m_editRes,
                                               this, m_editItem->GetText (column),
                                               wxPoint (x, y), wxSize (w, h), style);
    m_editControl->SelectAll();
    m_editControl->SetFocus();
}

void wxTreeListMainWindow::OnRenameTimer() {
    EditLabel (m_curItem, GetCurrentColumn());
}

void wxTreeListMainWindow::OnRenameAccept(bool isCancelled) {

    // TODO if the validator fails this causes a crash
    wxTreeEvent le( wxEVT_COMMAND_TREE_END_LABEL_EDIT, 0 );
    le.SetLabel( m_editRes );
    le.SetEditCanceled(isCancelled);
    le.SetInt(m_editCol);
    SendEvent(0, m_editItem, &le); if (! isCancelled  && le.IsAllowed())
    {
        SetItemText (m_editItem, le.GetInt(), le.GetLabel());
    }
}

void wxTreeListMainWindow::EndEdit(bool isCancelled) {
    if (m_editControl) { m_editControl->EndEdit(true); }
}

void wxTreeListMainWindow::OnMouse (wxMouseEvent &event) {
bool mayDrag = true;
bool maySelect = true;  // may change selection
bool mayClick = true;  // may process DOWN clicks to expand, send click events
bool mayDoubleClick = true;  // implies mayClick
bool bSkip = true;

    // send event to user code
    if (m_owner->GetEventHandler()->ProcessEvent(event)) return; // handled (and not skipped) in user code
    if (!m_rootItem) return;


// ---------- DETERMINE EVENT ----------
/*
wxLogMessage("OnMouse: LMR down=<%d, %d, %d> up=<%d, %d, %d> LDblClick=<%d> dragging=<%d>",
    event.LeftDown(), event.MiddleDown(), event.RightDown(),
    event.LeftUp(), event.MiddleUp(), event.RightUp(),
    event.LeftDClick(), event.Dragging());
*/
    wxPoint p = wxPoint (event.GetX(), event.GetY());
    int flags = 0;
    wxTreeListItem *item = m_rootItem->HitTest (CalcUnscrolledPosition (p),
                                                this, flags, m_curColumn, 0);
    bool bCrosshair = (item && item->HasPlus() && (flags & wxTREE_HITTEST_ONITEMBUTTON));
    // we were dragging
    if (m_isDragging) {
        maySelect = mayDoubleClick = false;
    }
    // we are starting or continuing to drag
    if (event.Dragging()) {
        maySelect = mayDoubleClick = mayClick = false;
    }
    // crosshair area is special
    if (bCrosshair) {
        // left click does not select
        if (event.LeftDown()) maySelect = false;
        // double click is ignored
        mayDoubleClick = false;
    }
    // double click only if simple click
    if (mayDoubleClick) mayDoubleClick = mayClick;
    // selection conditions --remember also that selection exludes editing
    if (maySelect) maySelect = mayClick;  // yes, select/unselect requires a click
    if (maySelect) {

        // multiple selection mode complicates things, sometimes we
        //  select on button-up instead of down:
        if (HasFlag(wxTR_MULTIPLE)) {

            // CONTROL/SHIFT key used, don't care about anything else, will
            //  toggle on key down
            if (event.ControlDown() || event.ShiftDown()) {
                maySelect = maySelect && (event.LeftDown() || event.RightDown());
                m_lastOnSame = false;  // prevent editing when keys are used

            // already selected item: to allow drag or contextual menu for multiple
            //  items, we only select/unselect on click-up --and only on LEFT
            // click, right is reserved for contextual menu
            } else if ((item != NULL && item->IsSelected())) {
                maySelect = maySelect && event.LeftUp();

            // non-selected items: select on click-down like simple select (so
            //  that a right-click contextual menu may be chained)
            } else {
                maySelect = maySelect && (event.LeftDown() || event.RightDown());
            }

        // single-select is simply on left or right click-down
        } else {
            maySelect = maySelect && (event.LeftDown() || event.RightDown());
        }
    }


// ----------  GENERAL ACTIONS  ----------

    // set focus if window clicked
    if (event.LeftDown() || event.MiddleDown() || event.RightDown()) SetFocus();

    // tooltip change ?
    if (item != m_toolTipItem) {

        // not over an item, use global tip
        if (item == NULL) {
            m_toolTipItem = NULL;
            wxScrolledWindow::SetToolTip(m_toolTip);

        // over an item
        } else {
            const wxString *tip = item->GetToolTip();

            // is there an item-specific tip ?
            if (tip) {
                m_toolTipItem = item;
                wxScrolledWindow::SetToolTip(*tip);

            // no item tip, but we are in item-specific mode (SetItemToolTip()
            //  was called after SetToolTip() )
            } else if (m_isItemToolTip) {
                m_toolTipItem = item;
                wxScrolledWindow::SetToolTip(wxString());

            // no item tip, display global tip instead; item change ignored
            } else if (m_toolTipItem != NULL) {
                m_toolTipItem = NULL;
                wxScrolledWindow::SetToolTip(m_toolTip);
            }
        }
    }


// ----------  HANDLE SIMPLE-CLICKS  (selection change, contextual menu) ----------
    if (mayClick) {

        // 2nd left-click on an item might trigger edit
        if (event.LeftDown()) m_lastOnSame = (item == m_curItem);

        // left-click on haircross is expand (and no select)
        if (bCrosshair && event.LeftDown()) {

            bSkip = false;

            // note that we only toggle the item for a single click, double
            // click on the button doesn't do anything
            Toggle (item);
        }

        if (maySelect) {
            bSkip = false;

            // set / remember item at shift down before current item gets changed
            if (event.LeftDown() && HasFlag(wxTR_MULTIPLE) && event.ShiftDown())  {
                if (!m_shiftItem) m_shiftItem = m_curItem;
            }else{
                m_shiftItem = (wxTreeListItem*)NULL;
            }

            // how is selection altered
            // keep or discard already selected ?
            bool unselect_others = ! (HasFlag(wxTR_MULTIPLE) && (
                event.ShiftDown()
             || event.ControlDown()
            ));

            // check is selection change is not vetoed
            if (SelectItem(item, m_shiftItem, unselect_others)) {
                // make the new item the current item
                EnsureVisible (item);
                SetCurrentItem(item);
            }
        }

        // generate click & menu events
        if (event.MiddleDown()) {
            // our own event to set point
            wxTreeEvent nevent(0, 0);
            nevent.SetPoint(p);
            nevent.SetInt(m_curColumn);
            bSkip = false;
            SendEvent(wxEVT_COMMAND_TREE_ITEM_MIDDLE_CLICK, item, &nevent);
        }
        if (event.RightDown()) {
            // our own event to set point
            wxTreeEvent nevent(0, 0);
            nevent.SetPoint(p);
            nevent.SetInt(m_curColumn);
            bSkip = false;
            SendEvent(wxEVT_COMMAND_TREE_ITEM_RIGHT_CLICK, item, &nevent);
        }
        if (event.RightUp()) {
            // our own event to set point
            wxTreeEvent nevent(0, 0);
            nevent.SetPoint(p);
            nevent.SetInt(m_curColumn);
            SendEvent(wxEVT_COMMAND_TREE_ITEM_MENU, item, &nevent);
        }

        // if 2nd left click finishes on same item, will edit it
        if (m_lastOnSame && event.LeftUp()) {
            if ((item == m_curItem) && (m_curColumn != -1) &&
                (m_owner->GetHeaderWindow()->IsColumnEditable (m_curColumn)) &&
                (flags & (wxTREE_HITTEST_ONITEMLABEL | wxTREE_HITTEST_ONITEMCOLUMN))
            ){
                m_editTimer->Start (RENAME_TIMER_TICKS, wxTIMER_ONE_SHOT);
                bSkip = false;
            }
            m_lastOnSame = false;
        }
    }


// ----------  HANDLE DOUBLE-CLICKS  ----------
    if (mayDoubleClick && event.LeftDClick()) {

        bSkip = false;

        // double clicking should not start editing the item label
        m_editTimer->Stop();
        m_lastOnSame = false;

        // selection reset to that single item which was double-clicked
        if (SelectItem(item, (wxTreeItemId*)NULL, true)) {  // unselect others --return false if vetoed

            // selection change not vetoed, send activate event
            if (! SendEvent(wxEVT_COMMAND_TREE_ITEM_ACTIVATED, item)) {

                // if the user code didn't process the activate event,
                // handle it ourselves by toggling the item when it is
                // double clicked
                if (item && item->HasPlus()) Toggle(item);
            }
        }
    }


// ----------  HANDLE DRAGGING  ----------
// NOTE: drag itself makes no change to selection
    if (mayDrag) {  // actually this is always true

        // CASE 1: we were dragging => continue, end, abort
        if (m_isDragging) {

            // CASE 1.1: click aborts drag:
            if (event.LeftDown() || event.MiddleDown() || event.RightDown()) {

                bSkip = false;

                // stop dragging
                m_isDragStarted = m_isDragging = false;
                if (HasCapture()) ReleaseMouse();
                RefreshSelected();

            // CASE 1.2: still dragging
            } else if (event.Dragging()) {

                ;; // nothing to do

            // CASE 1.3: dragging now ends normally
            } else {

                bSkip = false;

                // stop dragging
                m_isDragStarted = m_isDragging = false;
                if (HasCapture()) ReleaseMouse();
                RefreshSelected();

                // send drag end event
                // our own event to set point
                wxTreeEvent nevent(0, 0);
                nevent.SetPoint(p);
                nevent.SetInt(m_curColumn);
                SendEvent(wxEVT_COMMAND_TREE_END_DRAG, item, &nevent);
            }

        // CASE 2: not were not dragging => continue, start
        } else if (event.Dragging()) {

            // We will really start dragging if we've moved beyond a few pixels
            if (m_isDragStarted) {
                const int tolerance = 3;
                int dx = abs(p.x - m_dragStartPos.x);
                int dy = abs(p.y - m_dragStartPos.y);
                if (dx <= tolerance && dy <= tolerance)
                    return;
            // determine drag start
            } else {
                m_dragStartPos = p;
                m_dragCol = GetCurrentColumn();
                m_dragItem = item;
                m_isDragStarted = true;
                return;
            }

            bSkip = false;

            // we are now dragging
            m_isDragging = true;
            RefreshSelected();
            CaptureMouse(); // TODO: usefulness unclear

            wxTreeEvent nevent(0, 0);
            nevent.SetPoint(p);
            nevent.SetInt(m_dragCol);
            nevent.Veto();
            SendEvent(event.LeftIsDown()
                                  ? wxEVT_COMMAND_TREE_BEGIN_DRAG
                                  : wxEVT_COMMAND_TREE_BEGIN_RDRAG,
                      m_dragItem, &nevent);
        }
    }


    if (bSkip) event.Skip();
}


void wxTreeListMainWindow::OnIdle (wxIdleEvent &WXUNUSED(event)) {
    /* after all changes have been done to the tree control,
     * we actually redraw the tree when everything is over */

    if (!m_dirty) return;

    m_dirty = false;

    CalculatePositions();
    Refresh();
    AdjustMyScrollbars();
}

void wxTreeListMainWindow::OnScroll (wxScrollWinEvent& event) {

    // send event to wxTreeListCtrl (for user code)
    if (m_owner->GetEventHandler()->ProcessEvent(event)) return; // handled (and not skipped) in user code

    // TODO
    HandleOnScroll( event );

    if(event.GetOrientation() == wxHORIZONTAL) {
        m_owner->GetHeaderWindow()->Refresh();
        m_owner->GetHeaderWindow()->Update();
    }
}

void wxTreeListMainWindow::CalculateSize (wxTreeListItem *item, wxDC &dc) {
    wxCoord text_w = 0;
    wxCoord text_h = 0;

    dc.SetFont (GetItemFont (item));
    dc.GetTextExtent (!item->GetText(m_main_column).empty()
            ? item->GetText (m_main_column)
            : _T(" "),  // blank to avoid zero height and no highlight width
        &text_w, &text_h);
    // restore normal font
    dc.SetFont (m_normalFont);

    int max_h = (m_imgHeight > text_h) ? m_imgHeight : text_h;
    if (max_h < 30) { // add 10% space if greater than 30 pixels
        max_h += 2; // minimal 2 pixel space
    }else{
        max_h += max_h / 10; // otherwise 10% space
    }

    item->SetHeight (max_h);
    if (max_h > m_lineHeight) m_lineHeight = max_h;
    item->SetWidth(m_imgWidth + text_w+2);
}

// -----------------------------------------------------------------------------
void wxTreeListMainWindow::CalculateLevel (wxTreeListItem *item, wxDC &dc,
                                           int level, int &y, int x_colstart) {

    // calculate position of vertical lines
    int x = x_colstart + MARGIN; // start of column
    if (HasFlag(wxTR_LINES_AT_ROOT)) x += LINEATROOT; // space for lines at root
    if (HasButtons()) {
        x += (m_btnWidth-m_btnWidth2); // half button space
    }else{
        x += (m_indent-m_indent/2);
    }
    if (HasFlag(wxTR_HIDE_ROOT)) {
        x += m_indent * (level-1); // indent but not level 1
    }else{
        x += m_indent * level; // indent according to level
    }

    // a hidden root is not evaluated, but its children are always
    if (HasFlag(wxTR_HIDE_ROOT) && (level == 0)) goto Recurse;

    CalculateSize( item, dc );

    // set its position
    item->SetX (x);
    item->SetY (y);
    y += GetLineHeight(item);

    // we don't need to calculate collapsed branches
    if ( !item->IsExpanded() ) return;

Recurse:
    wxArrayTreeListItems& children = item->GetChildren();
    long n, count = (long)children.Count();
    ++level;
    for (n = 0; n < count; ++n) {
        CalculateLevel( children[n], dc, level, y, x_colstart );  // recurse
    }
}

void wxTreeListMainWindow::CalculatePositions() {
    if ( !m_rootItem ) return;

    wxClientDC dc(this);
    PrepareDC( dc );

    dc.SetFont( m_normalFont );

    dc.SetPen( m_dottedPen );
    //if(GetImageList() == NULL)
    // m_lineHeight = (int)(dc.GetCharHeight() + 4);

    int y = 2;
    int x_colstart = 0;
    for (int i = 0; i < (int)GetMainColumn(); ++i) {
        if (!m_owner->GetHeaderWindow()->IsColumnShown(i)) continue;
        x_colstart += m_owner->GetHeaderWindow()->GetColumnWidth(i);
    }
    CalculateLevel( m_rootItem, dc, 0, y, x_colstart ); // start recursion
}

void wxTreeListMainWindow::RefreshSubtree (wxTreeListItem *item) {
    if (m_dirty) return;

    wxClientDC dc(this);
    PrepareDC(dc);

    int cw = 0;
    int ch = 0;
    GetVirtualSize( &cw, &ch );

    wxRect rect;
    rect.x = dc.LogicalToDeviceX( 0 );
    rect.width = cw;
    rect.y = dc.LogicalToDeviceY( item->GetY() - 2 );
    rect.height = ch;

    Refresh (true, &rect );
    AdjustMyScrollbars();
}

void wxTreeListMainWindow::RefreshLine (wxTreeListItem *item) {
    if (m_dirty) return;

    wxClientDC dc(this);
    PrepareDC( dc );

    int cw = 0;
    int ch = 0;
    GetVirtualSize( &cw, &ch );

    wxRect rect;
    rect.x = dc.LogicalToDeviceX( 0 );
    rect.y = dc.LogicalToDeviceY( item->GetY() );
    rect.width = cw;
    rect.height = GetLineHeight(item); //dc.GetCharHeight() + 6;

    Refresh (true, &rect);
}

void wxTreeListMainWindow::RefreshSelected() {
    // TODO: this is awfully inefficient, we should keep the list of all
    //       selected items internally, should be much faster
    if (m_rootItem) {
        RefreshSelectedUnder (m_rootItem);
    }
}

void wxTreeListMainWindow::RefreshSelectedUnder (wxTreeListItem *item) {
    if (item->IsSelected()) {
        RefreshLine (item);
    }

    const wxArrayTreeListItems& children = item->GetChildren();
    long count = (long)children.GetCount();
    for (long n = 0; n < count; n++ ) {
        RefreshSelectedUnder (children[n]);
    }
}

// ----------------------------------------------------------------------------
// changing colours: we need to refresh the tree control
// ----------------------------------------------------------------------------

bool wxTreeListMainWindow::SetBackgroundColour (const wxColour& colour) {
    if (!wxWindow::SetBackgroundColour(colour)) return false;

    Refresh();
    return true;
}

bool wxTreeListMainWindow::SetForegroundColour (const wxColour& colour) {
    if (!wxWindow::SetForegroundColour(colour)) return false;

    Refresh();
    return true;
}

void wxTreeListMainWindow::SetItemText (const wxTreeItemId& itemId, int column, const wxString& text) {
    wxCHECK_RET (itemId.IsOk(), _T("invalid tree item"));

    if (this->IsFrozen())
    {
        wxTreeListItem *item = (wxTreeListItem*)itemId.m_pItem;
        item->SetText(column, text);
        m_dirty = true;
    }
    else
    {
        wxClientDC dc(this);
        wxTreeListItem *item = (wxTreeListItem*)itemId.m_pItem;
        item->SetText(column, text);
        CalculateSize(item, dc);
        RefreshLine(item);
    };
}

wxString wxTreeListMainWindow::GetItemText (const wxTreeItemId& itemId, int column) const {
    wxCHECK_MSG (itemId.IsOk(), _T(""), _T("invalid tree item") );

    if( IsVirtual() )   return m_owner->OnGetItemText(((wxTreeListItem*) itemId.m_pItem)->GetData(),column);
    else                return ((wxTreeListItem*) itemId.m_pItem)->GetText (column);
}

wxString wxTreeListMainWindow::GetItemText (wxTreeItemData* item, int column) const {
   wxASSERT_MSG( IsVirtual(), _T("can be used only with virtual control") );
   return m_owner->OnGetItemText(item, column);
}

void wxTreeListMainWindow::SetFocus() {
    wxWindow::SetFocus();
}


int wxTreeListMainWindow::GetItemWidth (int column, wxTreeListItem *item) {
    if (!item) return 0;

    // determine item width
    int w = 0, h = 0;
    wxFont font = GetItemFont (item);
    GetTextExtent (item->GetText (column), &w, &h, NULL, NULL, font.Ok()? &font: NULL);
    w += 2*MARGIN;

    // calculate width
    int width = w + 2*MARGIN;
    if (column == GetMainColumn()) {
        width += MARGIN;
        if (HasFlag(wxTR_LINES_AT_ROOT)) width += LINEATROOT;
        if (HasButtons()) width += m_btnWidth + LINEATROOT;
        if (item->GetCurrentImage() != NO_IMAGE) width += m_imgWidth;

        // count indent level
        int level = 0;
        wxTreeListItem *parent = item->GetItemParent();
        wxTreeListItem *root = (wxTreeListItem*)GetRootItem().m_pItem;
        while (parent && (!HasFlag(wxTR_HIDE_ROOT) || (parent != root))) {
            level++;
            parent = parent->GetItemParent();
        }
        if (level) width += level * GetIndent();
    }

    return width;
}

int wxTreeListMainWindow::GetBestColumnWidth (int column, wxTreeItemId parent) {
    int maxWidth, h;
    GetClientSize (&maxWidth, &h);
    int width = 0;

    // get root if on item
    if (!parent.IsOk()) parent = GetRootItem();

    // add root width
    if (!HasFlag(wxTR_HIDE_ROOT)) {
        int w = GetItemWidth (column, (wxTreeListItem*)parent.m_pItem);
        if (width < w) width = w;
        if (width > maxWidth) return maxWidth;
    }

    wxTreeItemIdValue cookie = 0;
    wxTreeItemId item = GetFirstChild (parent, cookie);
    while (item.IsOk()) {
        int w = GetItemWidth (column, (wxTreeListItem*)item.m_pItem);
        if (width < w) width = w;
        if (width > maxWidth) return maxWidth;

        // check the children of this item
        if (((wxTreeListItem*)item.m_pItem)->IsExpanded()) {
            int w = GetBestColumnWidth (column, item);
            if (width < w) width = w;
            if (width > maxWidth) return maxWidth;
        }

        // next sibling
        item = GetNextChild (parent, cookie);
    }

    return width;
}


bool wxTreeListMainWindow::SendEvent(wxEventType event_type, wxTreeListItem *item, wxTreeEvent *event) {
wxTreeEvent nevent (event_type, 0);

    if (event == NULL) {
        event = &nevent;
        event->SetInt (m_curColumn); // the mouse colum
    } else if (event_type) {
        event->SetEventType(event_type);
    }

    event->SetEventObject (m_owner);
    event->SetId(m_owner->GetId());
    if (item) {
        event->SetItem (item);
    }

    return m_owner->GetEventHandler()->ProcessEvent (*event);
}

#if wxCHECK_VERSION(3,1,3)
void wxTreeListMainWindow::OnDpiChanged(wxDPIChangedEvent& e)
{
    m_dirty = true;
    m_lineHeight = LINEHEIGHT;
    if (m_imageListNormal)
    {
        CalculateLineHeight();
    };
    Refresh();
}
#endif

//-----------------------------------------------------------------------------
//  wxTreeListCtrl
//-----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxTreeListCtrl, wxControl);

BEGIN_EVENT_TABLE(wxTreeListCtrl, wxControl)
    EVT_SIZE(wxTreeListCtrl::OnSize)
END_EVENT_TABLE();

bool wxTreeListCtrl::Create(wxWindow *parent, wxWindowID id,
                            const wxPoint& pos,
                            const wxSize& size,
                            long style, const wxValidator &validator,
                            const wxString& name)
{
    long main_style = style & ~(wxBORDER_SIMPLE | wxBORDER_SUNKEN | wxBORDER_DOUBLE |
                                wxBORDER_RAISED | wxBORDER_STATIC);
    main_style |= wxWANTS_CHARS ;
    long ctrl_style = style & ~(wxVSCROLL|wxHSCROLL);

    if (!wxControl::Create(parent, id, pos, size, ctrl_style, validator, name)) {
       return false;
    }
    m_main_win = new wxTreeListMainWindow (this, -1, wxPoint(0, 0), size,
                                           main_style, validator);
    m_header_win = new wxTreeListHeaderWindow (this, -1, m_main_win,
                                               wxPoint(0, 0), wxDefaultSize,
                                               wxTAB_TRAVERSAL);
    CalculateAndSetHeaderHeight();
    return true;
}

void wxTreeListCtrl::CalculateAndSetHeaderHeight()
{
    if (m_header_win) {

        // we use 'g' to get the descent, too
        int h;
#ifdef __WXMSW__
        h = (int)(wxRendererNative::Get().GetHeaderButtonHeight(m_header_win) * 0.8) + 2;
#else
        h = wxRendererNative::Get().GetHeaderButtonHeight(m_header_win);
#endif

        // only update if changed
        if (h != m_headerHeight) {
            m_headerHeight = h;
            DoHeaderLayout();
        }
    }
}

void wxTreeListCtrl::DoHeaderLayout()
{
    int w, h;
    GetClientSize(&w, &h);
    if (m_header_win) {
        m_header_win->SetSize (0, 0, w, m_headerHeight);
        m_header_win->Refresh();
    }
    if (m_main_win) {
        m_main_win->SetSize (0, m_headerHeight, w, h - m_headerHeight);
    }
}

void wxTreeListCtrl::OnSize(wxSizeEvent& WXUNUSED(event))
{
    DoHeaderLayout();
}

size_t wxTreeListCtrl::GetCount() const { return m_main_win->GetCount(); }

unsigned int wxTreeListCtrl::GetIndent() const
{ return m_main_win->GetIndent(); }

void wxTreeListCtrl::SetIndent(unsigned int indent)
{ m_main_win->SetIndent(indent); }

unsigned int wxTreeListCtrl::GetLineSpacing() const
{ return m_main_win->GetLineSpacing(); }

void wxTreeListCtrl::SetLineSpacing(unsigned int spacing)
{ m_main_win->SetLineSpacing(spacing); }

wxImageList* wxTreeListCtrl::GetImageList() const
{ return m_main_win->GetImageList(); }

wxImageList* wxTreeListCtrl::GetStateImageList() const
{ return m_main_win->GetStateImageList(); }

wxImageList* wxTreeListCtrl::GetButtonsImageList() const
{ return m_main_win->GetButtonsImageList(); }

void wxTreeListCtrl::SetImageList(wxImageList* imageList)
{ m_main_win->SetImageList(imageList); }

void wxTreeListCtrl::SetStateImageList(wxImageList* imageList)
{ m_main_win->SetStateImageList(imageList); }

void wxTreeListCtrl::SetButtonsImageList(wxImageList* imageList)
{ m_main_win->SetButtonsImageList(imageList); }

void wxTreeListCtrl::AssignImageList(wxImageList* imageList)
{ m_main_win->AssignImageList(imageList); }

void wxTreeListCtrl::AssignStateImageList(wxImageList* imageList)
{ m_main_win->AssignStateImageList(imageList); }

void wxTreeListCtrl::AssignButtonsImageList(wxImageList* imageList)
{ m_main_win->AssignButtonsImageList(imageList); }



wxString wxTreeListCtrl::GetItemText(const wxTreeItemId& item, int column) const
{ return m_main_win->GetItemText (item, column); }

int wxTreeListCtrl::GetItemImage(const wxTreeItemId& item, wxTreeItemIcon which) const
{ return m_main_win->GetItemImage(item, which); }
int wxTreeListCtrl::GetItemImage(const wxTreeItemId& item, int column) const
{ return m_main_win->GetItemImage(item, column); }

wxTreeItemData* wxTreeListCtrl::GetItemData(const wxTreeItemId& item) const
{ return m_main_win->GetItemData(item); }
wxTreeItemData* wxTreeListCtrl::GetItemData(const wxTreeItemId& item, int column) const
{ return m_main_win->GetItemData(item, column); }

bool wxTreeListCtrl::GetItemBold(const wxTreeItemId& item) const
{ return m_main_win->GetItemBold(item); }
bool wxTreeListCtrl::GetItemBold(const wxTreeItemId& item, int column) const
{ return m_main_win->GetItemBold(item, column); }

wxColour wxTreeListCtrl::GetItemTextColour(const wxTreeItemId& item) const
{ return m_main_win->GetItemTextColour(item); }
wxColour wxTreeListCtrl::GetItemTextColour(const wxTreeItemId& item, int column) const
{ return m_main_win->GetItemTextColour(item, column); }

wxColour wxTreeListCtrl::GetItemBackgroundColour(const wxTreeItemId& item) const
{ return m_main_win->GetItemBackgroundColour(item); }
wxColour wxTreeListCtrl::GetItemBackgroundColour(const wxTreeItemId& item, int column) const
{ return m_main_win->GetItemBackgroundColour(item, column); }

wxFont wxTreeListCtrl::GetItemFont(const wxTreeItemId& item) const
{ return m_main_win->GetItemFont(item); }
wxFont wxTreeListCtrl::GetItemFont(const wxTreeItemId& item, int column) const
{ return m_main_win->GetItemFont(item, column); }



void wxTreeListCtrl::SetItemHasChildren(const wxTreeItemId& item, bool has)
{ m_main_win->SetItemHasChildren(item, has); }

void wxTreeListCtrl::SetItemText(const wxTreeItemId& item, int column, const wxString& text)
{ m_main_win->SetItemText (item, column, text); }

void wxTreeListCtrl::SetItemImage(const wxTreeItemId& item, int image, wxTreeItemIcon which)
{ m_main_win->SetItemImage(item, image, which); }
void wxTreeListCtrl::SetItemImage(const wxTreeItemId& item, int column, int image)
{ m_main_win->SetItemImage(item, column, image); }

void wxTreeListCtrl::SetItemData(const wxTreeItemId& item,             wxTreeItemData* data)
{ m_main_win->SetItemData(item, data); }
void wxTreeListCtrl::SetItemData(const wxTreeItemId& item, int column, wxTreeItemData* data)
{ m_main_win->SetItemData(item, column, data); }

void wxTreeListCtrl::SetItemBold(const wxTreeItemId& item,             bool bold)
{ m_main_win->SetItemBold(item, bold); }
void wxTreeListCtrl::SetItemBold(const wxTreeItemId& item, int column, bool bold)
{ m_main_win->SetItemBold(item, column, bold); }

void wxTreeListCtrl::SetItemTextColour(const wxTreeItemId& item,              const wxColour& colour)
{ m_main_win->SetItemTextColour(item, colour); }
void wxTreeListCtrl::SetItemTextColour(const wxTreeItemId& item, int column, const wxColour& colour)
{ m_main_win->SetItemTextColour(item, column, colour); }

void wxTreeListCtrl::SetItemBackgroundColour(const wxTreeItemId& item,             const wxColour& colour)
{ m_main_win->SetItemBackgroundColour(item, colour); }
void wxTreeListCtrl::SetItemBackgroundColour(const wxTreeItemId& item, int column, const wxColour& colour)
{ m_main_win->SetItemBackgroundColour(item, column, colour); }

void wxTreeListCtrl::SetItemFont(const wxTreeItemId& item,             const wxFont& font)
{ m_main_win->SetItemFont(item, font); }
void wxTreeListCtrl::SetItemFont(const wxTreeItemId& item, int column, const wxFont& font)
{ m_main_win->SetItemFont(item, column, font); }



bool wxTreeListCtrl::SetFont(const wxFont& font)
{
    if (m_header_win) {
        m_header_win->SetFont(font);
        CalculateAndSetHeaderHeight();
        m_header_win->Refresh();
    }
    if (m_main_win) {
        return m_main_win->SetFont(font);
    }else{
        return false;
    }
}

void wxTreeListCtrl::SetWindowStyleFlag(long style)
{
    if (m_main_win)
    {
        long main_style = style & ~(wxBORDER_SIMPLE | wxBORDER_SUNKEN | wxBORDER_DOUBLE | wxBORDER_RAISED | wxBORDER_STATIC);
        main_style |= wxWANTS_CHARS;
        m_main_win->SetWindowStyle(main_style);
    };
    m_windowStyle = style & ~(wxVSCROLL | wxHSCROLL);
    // TODO: provide something like wxTL_NO_HEADERS to hide m_header_win
}

long wxTreeListCtrl::GetWindowStyleFlag() const
{
    long style = m_windowStyle;
    if(m_main_win)
        style |= m_main_win->GetWindowStyle();
    return style;
}

bool wxTreeListCtrl::IsVisible(const wxTreeItemId& item, bool fullRow, bool within) const
{ return m_main_win->IsVisible(item, fullRow, within); }

bool wxTreeListCtrl::HasChildren(const wxTreeItemId& item) const
{ return m_main_win->HasChildren(item); }

bool wxTreeListCtrl::IsExpanded(const wxTreeItemId& item) const
{ return m_main_win->IsExpanded(item); }

bool wxTreeListCtrl::IsSelected(const wxTreeItemId& item) const
{ return m_main_win->IsSelected(item); }

size_t wxTreeListCtrl::GetChildrenCount(const wxTreeItemId& item, bool rec)
{ return m_main_win->GetChildrenCount(item, rec); }

wxTreeItemId wxTreeListCtrl::GetRootItem() const
{ return m_main_win->GetRootItem(); }

wxTreeItemId wxTreeListCtrl::GetSelection() const
{ return m_main_win->GetSelection(); }

size_t wxTreeListCtrl::GetSelections(wxArrayTreeItemIds& arr) const
{ return m_main_win->GetSelections(arr); }

wxTreeItemId wxTreeListCtrl::GetItemParent(const wxTreeItemId& item) const
{ return m_main_win->GetItemParent(item); }

wxTreeItemId wxTreeListCtrl::GetFirstChild (const wxTreeItemId& item,
                                            wxTreeItemIdValue& cookie) const
{ return m_main_win->GetFirstChild(item, cookie); }

wxTreeItemId wxTreeListCtrl::GetNextChild (const wxTreeItemId& item,
                                           wxTreeItemIdValue& cookie) const
{ return m_main_win->GetNextChild(item, cookie); }

wxTreeItemId wxTreeListCtrl::GetPrevChild (const wxTreeItemId& item,
                                           wxTreeItemIdValue& cookie) const
{ return m_main_win->GetPrevChild(item, cookie); }

wxTreeItemId wxTreeListCtrl::GetLastChild (const wxTreeItemId& item,
                                           wxTreeItemIdValue& cookie) const
{ return m_main_win->GetLastChild(item, cookie); }


wxTreeItemId wxTreeListCtrl::GetNextSibling(const wxTreeItemId& item) const
{ return m_main_win->GetNextSibling(item); }

wxTreeItemId wxTreeListCtrl::GetPrevSibling(const wxTreeItemId& item) const
{ return m_main_win->GetPrevSibling(item); }

wxTreeItemId wxTreeListCtrl::GetNext(const wxTreeItemId& item) const
{ return m_main_win->GetNext(item, true); }

wxTreeItemId wxTreeListCtrl::GetPrev(const wxTreeItemId& item) const
{ return m_main_win->GetPrev(item, true); }

wxTreeItemId wxTreeListCtrl::GetFirstExpandedItem() const
{ return m_main_win->GetFirstExpandedItem(); }

wxTreeItemId wxTreeListCtrl::GetNextExpanded(const wxTreeItemId& item) const
{ return m_main_win->GetNextExpanded(item); }

wxTreeItemId wxTreeListCtrl::GetPrevExpanded(const wxTreeItemId& item) const
{ return m_main_win->GetPrevExpanded(item); }

wxTreeItemId wxTreeListCtrl::GetFirstVisibleItem(bool fullRow) const
{ return GetFirstVisible(fullRow); }
wxTreeItemId wxTreeListCtrl::GetFirstVisible(bool fullRow, bool within) const
{ return m_main_win->GetFirstVisible(fullRow, within); }

wxTreeItemId wxTreeListCtrl::GetLastVisible(bool fullRow, bool within) const
{ return m_main_win->GetLastVisible(fullRow, within); }

wxTreeItemId wxTreeListCtrl::GetNextVisible(const wxTreeItemId& item, bool fullRow, bool within) const
{ return m_main_win->GetNextVisible(item, fullRow, within); }

wxTreeItemId wxTreeListCtrl::GetPrevVisible(const wxTreeItemId& item, bool fullRow, bool within) const
{ return m_main_win->GetPrevVisible(item, fullRow, within); }

wxTreeItemId wxTreeListCtrl::AddRoot (const wxString& text, int image,
                                      int selectedImage, wxTreeItemData* data)
{ return m_main_win->AddRoot (text, image, selectedImage, data); }

wxTreeItemId wxTreeListCtrl::PrependItem(const wxTreeItemId& parent,
                                         const wxString& text, int image,
                                         int selectedImage,
                                         wxTreeItemData* data)
{ return m_main_win->PrependItem(parent, text, image, selectedImage, data); }

wxTreeItemId wxTreeListCtrl::InsertItem(const wxTreeItemId& parent,
                                        const wxTreeItemId& previous,
                                        const wxString& text, int image,
                                        int selectedImage,
                                        wxTreeItemData* data)
{
    return m_main_win->InsertItem(parent, previous, text, image,
                                  selectedImage, data);
}

wxTreeItemId wxTreeListCtrl::InsertItem(const wxTreeItemId& parent,
                                        size_t index,
                                        const wxString& text, int image,
                                        int selectedImage,
                                        wxTreeItemData* data)
{
    return m_main_win->InsertItem(parent, index, text, image,
                                  selectedImage, data);
}

wxTreeItemId wxTreeListCtrl::AppendItem(const wxTreeItemId& parent,
                                        const wxString& text, int image,
                                        int selectedImage,
                                        wxTreeItemData* data)
{ return m_main_win->AppendItem(parent, text, image, selectedImage, data); }

void wxTreeListCtrl::Delete(const wxTreeItemId& item)
{ m_main_win->Delete(item); }

void wxTreeListCtrl::DeleteChildren(const wxTreeItemId& item)
{ m_main_win->DeleteChildren(item); }

void wxTreeListCtrl::DeleteRoot()
{ m_main_win->DeleteRoot(); }

void wxTreeListCtrl::Expand(const wxTreeItemId& item)
{ m_main_win->Expand(item); }

void wxTreeListCtrl::ExpandAll(const wxTreeItemId& item)
{ m_main_win->ExpandAll(item); }

void wxTreeListCtrl::Collapse(const wxTreeItemId& item)
{ m_main_win->Collapse(item); }

void wxTreeListCtrl::CollapseAndReset(const wxTreeItemId& item)
{ m_main_win->CollapseAndReset(item); }

void wxTreeListCtrl::Toggle(const wxTreeItemId& item)
{ m_main_win->Toggle(item); }

void wxTreeListCtrl::Unselect()
{ m_main_win->Unselect(); }

void wxTreeListCtrl::UnselectAll()
{ m_main_win->UnselectAll(); }

bool wxTreeListCtrl::SelectItem(const wxTreeItemId& item, const wxTreeItemId& last,
                                bool unselect_others)
{ return m_main_win->SelectItem (item, last, unselect_others); }

void wxTreeListCtrl::SelectAll()
{ m_main_win->SelectAll(); }

void wxTreeListCtrl::EnsureVisible(const wxTreeItemId& item)
{ m_main_win->EnsureVisible(item); }

void wxTreeListCtrl::ScrollTo(const wxTreeItemId& item)
{ m_main_win->ScrollTo(item); }

wxTreeItemId wxTreeListCtrl::HitTest(const wxPoint& pos, int& flags, int& column)
{
    wxPoint p = pos;
    return m_main_win->HitTest (p, flags, column);
}

bool wxTreeListCtrl::GetBoundingRect(const wxTreeItemId& item, wxRect& rect,
                                     bool textOnly) const
{ return m_main_win->GetBoundingRect(item, rect, textOnly); }

void wxTreeListCtrl::EditLabel (const wxTreeItemId& item, int column)
    { m_main_win->EditLabel (item, column); }
void wxTreeListCtrl::EndEdit(bool isCancelled)
    { m_main_win->EndEdit(isCancelled); }

int wxTreeListCtrl::OnCompareItems(const wxTreeItemId& item1, const wxTreeItemId& item2)
{
    // do the comparison here and not in m_main_win in order to allow
    // override in child class
    return wxStrcmp(GetItemText(item1), GetItemText(item2));
}
int wxTreeListCtrl::OnCompareItems(const wxTreeItemId& item1, const wxTreeItemId& item2, int column)
{
    // do the comparison here and not in m_main_win in order to allow
    // override in child class
    return wxStrcmp(GetItemText(item1, column), GetItemText(item2, column));
}

void wxTreeListCtrl::SortChildren(const wxTreeItemId& item, int column, bool reverseOrder)
{ m_main_win->SortChildren(item, column, reverseOrder); }

wxTreeItemId wxTreeListCtrl::FindItem (const wxTreeItemId& item, int column, const wxString& str, int mode)
{ return m_main_win->FindItem (item, column, str, mode); }

void wxTreeListCtrl::SetDragItem (const wxTreeItemId& item)
{ m_main_win->SetDragItem (item); }

bool wxTreeListCtrl::SetBackgroundColour(const wxColour& colour)
{
    if (!m_main_win) return false;
    return m_main_win->SetBackgroundColour(colour);
}

bool wxTreeListCtrl::SetForegroundColour(const wxColour& colour)
{
    if (!m_main_win) return false;
    return m_main_win->SetForegroundColour(colour);
}

int wxTreeListCtrl::GetColumnCount() const
{ return m_main_win->GetColumnCount(); }

void wxTreeListCtrl::SetColumnWidth(int column, int width)
{
    m_header_win->SetColumnWidth (column, width);
    m_header_win->Refresh();
}

int wxTreeListCtrl::GetColumnWidth(int column) const
{ return m_header_win->GetColumnWidth(column); }

void wxTreeListCtrl::SetMainColumn(int column)
{ m_main_win->SetMainColumn(column); }

int wxTreeListCtrl::GetMainColumn() const
{ return m_main_win->GetMainColumn(); }

void wxTreeListCtrl::SetColumnText(int column, const wxString& text)
{
    m_header_win->SetColumnText (column, text);
    m_header_win->Refresh();
}

wxString wxTreeListCtrl::GetColumnText(int column) const
{ return m_header_win->GetColumnText(column); }

void wxTreeListCtrl::AddColumn(const wxTreeListColumnInfo& colInfo)
{
    m_header_win->AddColumn (colInfo);
    DoHeaderLayout();
}

void wxTreeListCtrl::InsertColumn(int before, const wxTreeListColumnInfo& colInfo)
{
    m_header_win->InsertColumn (before, colInfo);
    m_header_win->Refresh();
}

void wxTreeListCtrl::RemoveColumn(int column)
{
    m_header_win->RemoveColumn (column);
    m_header_win->Refresh();
}

void wxTreeListCtrl::SetColumn(int column, const wxTreeListColumnInfo& colInfo)
{
    m_header_win->SetColumn (column, colInfo);
    m_header_win->Refresh();
}

const wxTreeListColumnInfo& wxTreeListCtrl::GetColumn(int column) const
{ return m_header_win->GetColumn(column); }

wxTreeListColumnInfo wxTreeListCtrl::GetColumn(int column)
{ return m_header_win->GetColumn(column); }

void wxTreeListCtrl::SetColumnImage(int column, int image)
{
    m_header_win->SetColumn (column, GetColumn(column).SetImage(image));
    m_header_win->Refresh();
}

int wxTreeListCtrl::GetColumnImage(int column) const
{
    return m_header_win->GetColumn(column).GetImage();
}

void wxTreeListCtrl::SetColumnEditable(int column, bool shown)
{
    m_header_win->SetColumn (column, GetColumn(column).SetEditable(shown));
}

void wxTreeListCtrl::SetColumnShown(int column, bool shown)
{
    wxASSERT_MSG (column != GetMainColumn(), _T("The main column may not be hidden") );
    m_header_win->SetColumn (column, GetColumn(column).SetShown(GetMainColumn()==column? true: shown));
    m_header_win->Refresh();
}

bool wxTreeListCtrl::IsColumnEditable(int column) const
{
    return m_header_win->GetColumn(column).IsEditable();
}

bool wxTreeListCtrl::IsColumnShown(int column) const
{
    return m_header_win->GetColumn(column).IsShown();
}

void wxTreeListCtrl::SetColumnAlignment (int column, int flag)
{
    m_header_win->SetColumn(column, GetColumn(column).SetAlignment(flag));
    m_header_win->Refresh();
}

int wxTreeListCtrl::GetColumnAlignment(int column) const
{
    return m_header_win->GetColumn(column).GetAlignment();
}

void wxTreeListCtrl::Refresh(bool erase, const wxRect* rect)
{
    m_main_win->Refresh (erase, rect);
    m_header_win->Refresh (erase, rect);
}

void wxTreeListCtrl::SetFocus()
{ m_main_win->SetFocus(); }

wxSize wxTreeListCtrl::DoGetBestSize() const
{
    wxSize bestSizeHeader = m_header_win->GetBestSize();
    wxSize bestSizeMain = m_main_win->GetBestSize();
    return wxSize (bestSizeHeader.x > bestSizeMain.x ? bestSizeHeader.x : bestSizeMain.x, bestSizeHeader.y + bestSizeMain.y);
}

wxString wxTreeListCtrl::OnGetItemText( wxTreeItemData* WXUNUSED(item), long WXUNUSED(column)) const
{
    return wxEmptyString;
}

void wxTreeListCtrl::SetToolTip(const wxString& tip) {
    m_header_win->SetToolTip(tip);
    m_main_win->SetToolTip(tip);
}
void wxTreeListCtrl::SetToolTip(wxToolTip *tip) {
    m_header_win->SetToolTip(tip);
    m_main_win->SetToolTip(tip);
}

void wxTreeListCtrl::SetItemToolTip(const wxTreeItemId& item, const wxString &tip) {
    m_main_win->SetItemToolTip(item, tip);
}

void wxTreeListCtrl::SetCurrentItem(const wxTreeItemId& itemId) {
    m_main_win->SetCurrentItem(itemId);
}

void wxTreeListCtrl::SetItemParent(const wxTreeItemId& parent, const wxTreeItemId& item) {
    m_main_win->SetItemParent(parent, item);
}

//-----------------------------------------------------------------------------
// wxTreeListCtrlXmlHandler - XRC support for wxTreeListCtrl
//-----------------------------------------------------------------------------

#if wxUSE_XRC

IMPLEMENT_DYNAMIC_CLASS(wxTreeListCtrlXmlHandler, wxXmlResourceHandler)

wxTreeListCtrlXmlHandler::wxTreeListCtrlXmlHandler() : wxXmlResourceHandler() {

#define wxTR_NO_BUTTONS              0x0000     // for convenience
#define wxTR_HAS_BUTTONS             0x0001     // draw collapsed/expanded btns
#define wxTR_NO_LINES                0x0004     // don't draw lines at all
#define wxTR_LINES_AT_ROOT           0x0008     // connect top-level nodes
#define wxTR_TWIST_BUTTONS           0x0010     // still used by wxTreeListCtrl

#define wxTR_SINGLE                  0x0000     // for convenience
#define wxTR_MULTIPLE                0x0020     // can select multiple items
#define wxTR_EXTENDED                0x0040     // TODO: allow extended selection
#define wxTR_HAS_VARIABLE_ROW_HEIGHT 0x0080     // what it says

#define wxTR_EDIT_LABELS             0x0200     // can edit item labels
#define wxTR_ROW_LINES               0x0400     // put border around items
#define wxTR_HIDE_ROOT               0x0800     // don't display root node

#define wxTR_FULL_ROW_HIGHLIGHT      0x2000     // highlight full horz space

#ifdef __WXGTK20__
#define wxTR_DEFAULT_STYLE           (wxTR_HAS_BUTTONS | wxTR_NO_LINES)
#else
#define wxTR_DEFAULT_STYLE           (wxTR_HAS_BUTTONS | wxTR_LINES_AT_ROOT)
#endif

// wxTreeCtrl styles, taken from treebase.h
	XRC_ADD_STYLE(wxTR_NO_BUTTONS);
	XRC_ADD_STYLE(wxTR_HAS_BUTTONS);
	XRC_ADD_STYLE(wxTR_NO_LINES);
	XRC_ADD_STYLE(wxTR_LINES_AT_ROOT);
	XRC_ADD_STYLE(wxTR_TWIST_BUTTONS);

	XRC_ADD_STYLE(wxTR_SINGLE);
	XRC_ADD_STYLE(wxTR_MULTIPLE);
#if WXWIN_COMPATIBILITY_2_8
    // according to wxWidgets release notes, wxTR_EXTENDED is deprecated
    XRC_ADD_STYLE(wxTR_EXTENDED);
#endif // WXWIN_COMPATIBILITY_2_8
    XRC_ADD_STYLE(wxTR_HAS_VARIABLE_ROW_HEIGHT);

    XRC_ADD_STYLE(wxTR_EDIT_LABELS);
    XRC_ADD_STYLE(wxTR_ROW_LINES);
    XRC_ADD_STYLE(wxTR_HIDE_ROOT);

    XRC_ADD_STYLE(wxTR_FULL_ROW_HIGHLIGHT);

    XRC_ADD_STYLE(wxTR_DEFAULT_STYLE);

// wxTreeListCtrl-specific styles
    XRC_ADD_STYLE(wxTR_COLUMN_LINES);
    XRC_ADD_STYLE(wxTR_VIRTUAL);

// standard wxWidgets styles
	AddWindowStyles();
}

wxObject *wxTreeListCtrlXmlHandler::DoCreateResource() {
	XRC_MAKE_INSTANCE(tlc, wxTreeListCtrl);
	tlc->Create(m_parentAsWindow, GetID(), GetPosition(), GetSize(), GetStyle(), wxDefaultValidator, GetName());
    SetupWindow(tlc);
	return tlc;
}

bool wxTreeListCtrlXmlHandler::CanHandle(wxXmlNode * node) {
	return IsOfClass(node, wxT("TreeListCtrl"));
}

#endif  // wxUSE_XRC

} // namespace wxcode
