// scintilla-internal.h 
//
#ifndef __SCINTILLA_INTERNAL_H
#define __SCINTILLA_INTERNAL_H

#include "CaseConvert.h"
#include "UnicodeFromUTF8.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "PerLine.h"
#include "CallTip.h"
#include "KeyMap.h"
#include "ViewStyle.h"
#include "Decoration.h"
#include "Document.h"
#include "Selection.h"
#include "PositionCache.h"
#include "XPM.h"
//#include "EditModel.h"
//#include "MarginView.h"
//#include "EditView.h"
//#include "AutoComplete.h"
//#include "Editor.h"
//#include "ScintillaBase.h"
//#include "SparseVector.h"
//#include "RESearch.h"
//#include "ExternalLexer.h"

#if PLAT_WIN
	#define EXT_LEXER_DECL __stdcall
#else
	#define EXT_LEXER_DECL
#endif

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif
//
//
//
class Caret {
public:
	//bool   active;
	//bool   on;
	enum {
		fActive = 0x0001,
		fOn     = 0x0002
	};
	int    Flags;
	int    period;

	Caret();
};

class EditModel {
	// Private so EditModel objects can not be copied
	explicit EditModel(const EditModel &);
	EditModel & FASTCALL operator = (const EditModel &);
public:
	int    xOffset; ///< Horizontal scrolled amount in pixels
	SpecialRepresentations reprs;
	Caret  caret;
	SelectionPosition posDrag;
	Position braces[2];
	int    bracesMatchStyle;
	int    highlightGuideColumn;
	Selection sel;

	enum IMEInteraction { 
		imeWindowed, 
		imeInline 
	} imeInteraction;

	int    foldFlags;
	int    foldDisplayTextStyle;
	ContractionState cs;
	Range  hotspot; // Hotspot support
	int    hoverIndicatorPos;
	int    wrapWidth; // Wrapping support
	Document * pdoc;
	//bool   inOverstrike;
	//bool   trackLineWidth;
	//bool   primarySelection;
	enum {
		fInOverstrike     = 0x0001,
		fTrackLineWidth   = 0x0002,
		fPrimarySelection = 0x0004
	};
	long   EditModelFlags;

	EditModel();
	virtual ~EditModel();
	virtual int TopLineOfMain() const = 0;
	virtual Point GetVisibleOriginInMain() const = 0;
	virtual int LinesOnScreen() const = 0;
	virtual Range GetHotSpotRange() const = 0;
	ColourDesired SelectionBackground(const ViewStyle & vsDraw, bool main) const
	{
		return main ? ((EditModelFlags & fPrimarySelection) ? vsDraw.selColours.back : vsDraw.selBackground2) : vsDraw.selAdditionalBackground;
	}
};
//
//
//
void DrawWrapMarker(Surface *surface, PRectangle rcPlace, bool isEndMarker, ColourDesired wrapColour);

typedef void (*DrawWrapMarkerFn)(Surface *surface, PRectangle rcPlace, bool isEndMarker, ColourDesired wrapColour);
//
// MarginView draws the margins.
//
class MarginView {
public:
	Surface *pixmapSelMargin;
	Surface *pixmapSelPattern;
	Surface *pixmapSelPatternOffset1;
	// Highlight current folding block
	HighlightDelimiter highlightDelimiter;

	int wrapMarkerPaddingRight; // right-most pixel padding of wrap markers
	/** Some platforms, notably PLAT_CURSES, do not support Scintilla's native
	 * DrawWrapMarker function for drawing wrap markers. Allow those platforms to
	 * override it instead of creating a new method in the Surface class that
	 * existing platforms must implement as empty. */
	DrawWrapMarkerFn customDrawWrapMarker;

	MarginView();

	void DropGraphics(bool freeObjects);
	void AllocateGraphics(const ViewStyle &vsDraw);
	void RefreshPixMaps(Surface *surfaceWindow, WindowID wid, const ViewStyle &vsDraw);
	void PaintMargin(Surface *surface, int topLine, PRectangle rc, PRectangle rcMargin, const EditModel &model, const ViewStyle &vs);
};
//
//
//
struct PrintParameters {
	int magnification;
	int colourMode;
	WrapMode wrapState;
	PrintParameters();
};
/**
* The view may be drawn in separate phases.
*/
enum DrawPhase {
	drawBack = 0x1,
	drawIndicatorsBack = 0x2,
	drawText = 0x4,
	drawIndentationGuides = 0x8,
	drawIndicatorsFore = 0x10,
	drawSelectionTranslucent = 0x20,
	drawLineTranslucent = 0x40,
	drawFoldLines = 0x80,
	drawCarets = 0x100,
	drawAll = 0x1FF
};

bool ValidStyledText(const ViewStyle &vs, size_t styleOffset, const StyledText &st);
int WidestLineWidth(Surface *surface, const ViewStyle &vs, int styleOffset, const StyledText &st);
void DrawTextNoClipPhase(Surface *surface, PRectangle rc, const Style &style, XYPOSITION ybase, const char *s, int len, DrawPhase phase);
void DrawStyledText(Surface *surface, const ViewStyle &vs, int styleOffset, PRectangle rcText, const StyledText &st, size_t start, size_t length, DrawPhase phase);

typedef void (*DrawTabArrowFn)(Surface *surface, PRectangle rcTab, int ymid);
/**
* EditView draws the main text area.
*/
class EditView {
public:
	PrintParameters printParameters;
	PerLine *ldTabstops;
	int tabWidthMinimumPixels;
	bool hideSelection;
	bool drawOverstrikeCaret;
	/** In bufferedDraw mode, graphics operations are drawn to a pixmap and then copied to
	* the screen. This avoids flashing but is about 30% slower. */
	bool bufferedDraw;
	/** In phasesTwo mode, drawing is performed in two phases, first the background
	* and then the foreground. This avoids chopping off characters that overlap the next run.
	* In multiPhaseDraw mode, drawing is performed in multiple phases with each phase drawing
	* one feature over the whole drawing area, instead of within one line. This allows text to
	* overlap from one line to the next. */
	enum PhasesDraw { phasesOne, phasesTwo, phasesMultiple };
	PhasesDraw phasesDraw;
	int lineWidthMaxSeen;
	bool additionalCaretsBlink;
	bool additionalCaretsVisible;
	bool imeCaretBlockOverride;
	Surface *pixmapLine;
	Surface *pixmapIndentGuide;
	Surface *pixmapIndentGuideHighlight;
	LineLayoutCache llc;
	PositionCache posCache;
	int tabArrowHeight; // draw arrow heads this many pixels above/below line midpoint
	/** Some platforms, notably PLAT_CURSES, do not support Scintilla's native
	 * DrawTabArrow function for drawing tab characters. Allow those platforms to
	 * override it instead of creating a new method in the Surface class that
	 * existing platforms must implement as empty. */
	DrawTabArrowFn customDrawTabArrow;
	DrawWrapMarkerFn customDrawWrapMarker;

	EditView();
	virtual ~EditView();
	bool SetTwoPhaseDraw(bool twoPhaseDraw);
	bool SetPhasesDraw(int phases);
	bool LinesOverlap() const;
	void ClearAllTabstops();
	XYPOSITION NextTabstopPos(int line, XYPOSITION x, XYPOSITION tabWidth) const;
	bool ClearTabstops(int line);
	bool AddTabstop(int line, int x);
	int GetNextTabstop(int line, int x) const;
	void LinesAddedOrRemoved(int lineOfPos, int linesAdded);
	void DropGraphics(bool freeObjects);
	void AllocateGraphics(const ViewStyle &vsDraw);
	void RefreshPixMaps(Surface *surfaceWindow, WindowID wid, const ViewStyle &vsDraw);
	LineLayout *RetrieveLineLayout(int lineNumber, const EditModel &model);
	void LayoutLine(const EditModel &model, int line, Surface *surface, const ViewStyle &vstyle,
		LineLayout *ll, int width = LineLayout::wrapWidthInfinite);
	Point LocationFromPosition(Surface *surface, const EditModel &model, SelectionPosition pos, int topLine, const ViewStyle &vs, PointEnd pe);
	Range RangeDisplayLine(Surface *surface, const EditModel &model, int lineVisible, const ViewStyle &vs);
	SelectionPosition SPositionFromLocation(Surface *surface, const EditModel &model, PointDocument pt, bool canReturnInvalid,
		bool charPosition, bool virtualSpace, const ViewStyle &vs);
	SelectionPosition SPositionFromLineX(Surface *surface, const EditModel &model, int lineDoc, int x, const ViewStyle &vs);
	int DisplayFromPosition(Surface *surface, const EditModel &model, int pos, const ViewStyle &vs);
	int StartEndDisplayLine(Surface *surface, const EditModel &model, int pos, bool start, const ViewStyle &vs);
	void DrawIndentGuide(Surface *surface, int lineVisible, int lineHeight, int start, PRectangle rcSegment, bool highlight);
	void DrawEOL(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, PRectangle rcLine,
		int line, int lineEnd, int xStart, int subLine, XYACCUMULATOR subLineStart, ColourOptional background);
	void DrawFoldDisplayText(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
		int line, int xStart, PRectangle rcLine, int subLine, XYACCUMULATOR subLineStart, DrawPhase phase);
	void DrawAnnotation(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
		int line, int xStart, PRectangle rcLine, int subLine, DrawPhase phase);
	void DrawCarets(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, int line,
		int xStart, PRectangle rcLine, int subLine) const;
	void DrawBackground(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, PRectangle rcLine,
		Range lineRange, int posLineStart, int xStart, int subLine, ColourOptional background) const;
	void DrawForeground(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, int lineVisible,
		PRectangle rcLine, Range lineRange, int posLineStart, int xStart, int subLine, ColourOptional background);
	void DrawIndentGuidesOverEmpty(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll,
		int line, int lineVisible, PRectangle rcLine, int xStart, int subLine);
	void DrawLine(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, int line,
		int lineVisible, int xStart, PRectangle rcLine, int subLine, DrawPhase phase);
	void PaintText(Surface *surfaceWindow, const EditModel &model, PRectangle rcArea, PRectangle rcClient, const ViewStyle &vsDraw);
	void FillLineRemainder(Surface *surface, const EditModel &model, const ViewStyle &vsDraw, const LineLayout *ll, int line, PRectangle rcArea, int subLine) const;
	long FormatRange(bool draw, Sci_RangeToFormat *pfr, Surface *surface, Surface *surfaceMeasure, const EditModel &model, const ViewStyle &vs);
};
/**
* Convenience class to ensure LineLayout objects are always disposed.
*/
class AutoLineLayout {
	LineLayoutCache &llc;
	LineLayout *ll;
	AutoLineLayout &operator=(const AutoLineLayout &);
public:
	AutoLineLayout(LineLayoutCache &llc_, LineLayout *ll_) : llc(llc_), ll(ll_) {}
	~AutoLineLayout() 
	{
		llc.Dispose(ll);
		ll = 0;
	}
	LineLayout *operator->() const 
	{
		return ll;
	}
	operator LineLayout *() const 
	{
		return ll;
	}
	void Set(LineLayout *ll_) 
	{
		llc.Dispose(ll);
		ll = ll_;
	}
};
//
//
//
class AutoComplete {
private:
	bool active;
	char separator;
	char typesep; // Type seperator
	char reserve; // @alignment
	std::string stopChars;
	std::string fillUpChars;
	enum { 
		maxItemLen = 1000 
	};
	std::vector<int> sortMatrix;
public:
	ListBox *lb;
	int posStart;
	int startLen;
	/// Should autocompletion be canceled if editor's currentPos <= startPos?
	bool cancelAtStartPos;
	bool autoHide;
	bool dropRestOfWord;
	bool ignoreCase;
	bool chooseSingle;
	uint ignoreCaseBehaviour;
	int widthLBDefault;
	int heightLBDefault;
	/** SC_ORDER_PRESORTED:   Assume the list is presorted; selection will fail if it is not alphabetical<br />
	 *  SC_ORDER_PERFORMSORT: Sort the list alphabetically; start up performance cost for sorting<br />
	 *  SC_ORDER_CUSTOM:      Handle non-alphabetical entries; start up performance cost for generating a sorted lookup table
	 */
	int autoSort;

	AutoComplete();
	~AutoComplete();

	/// Is the auto completion list displayed?
	bool Active() const;

	/// Display the auto completion list positioned to be near a character position
	void Start(Window &parent, int ctrlID, int position, Point location, int startLen_, int lineHeight, bool unicodeMode, int technology);

	/// The stop chars are characters which, when typed, cause the auto completion list to disappear
	void SetStopChars(const char *stopChars_);
	bool IsStopChar(char ch);

	/// The fillup chars are characters which, when typed, fill up the selected word
	void SetFillUpChars(const char *fillUpChars_);
	bool IsFillUpChar(char ch);

	/// The separator character is used when interpreting the list in SetList
	void SetSeparator(char separator_);
	char GetSeparator() const;

	/// The typesep character is used for separating the word from the type
	void SetTypesep(char separator_);
	char GetTypesep() const;

	/// The list string contains a sequence of words separated by the separator character
	void SetList(const char *list);

	/// Return the position of the currently selected list item
	int GetSelection() const;

	/// Return the value of an item in the list
	std::string GetValue(int item) const;

	void Show(bool show);
	void Cancel();
	/// Move the current list element by delta, scrolling appropriately
	void Move(int delta);
	/// Select a list element that starts with word as the current element
	void Select(const char *word);
};
//
//
//
class Timer {
public:
	bool ticking;
	int ticksToWait;
	enum {
		tickSize = 100
	};
	TickerID tickerID;
	Timer();
};
//
//
//
class Idler {
public:
	bool state;
	IdlerID idlerID;
	Idler();
};
/**
 * When platform has a way to generate an event before painting,
 * accumulate needed styling range and other work items in
 * WorkNeeded to avoid unnecessary work inside paint handler
 */
class WorkNeeded {
public:
	enum workItems {
		workNone = 0,
		workStyle = 1,
		workUpdateUI = 2
	};
	enum workItems items;
	Position upTo;

	WorkNeeded() : items(workNone), upTo(0)
	{
	}
	void Reset()
	{
		items = workNone;
		upTo = 0;
	}
	void Need(workItems items_, Position pos)
	{
		if((items_ & workStyle) && (upTo < pos))
			upTo = pos;
		items = static_cast<workItems>(items | items_);
	}
};
/**
 * Hold a piece of text selected for copying or dragging, along with encoding and selection format information.
 */
class SelectionText {
public:
	SelectionText() : rectangular(false), lineCopy(false), codePage(0), characterSet(0)
	{
	}
	~SelectionText()
	{
	}
	void Clear()
	{
		s.clear();
		rectangular = false;
		lineCopy = false;
		codePage = 0;
		characterSet = 0;
	}
	void Copy(const std::string &s_, int codePage_, int characterSet_, bool rectangular_, bool lineCopy_)
	{
		s = s_;
		codePage = codePage_;
		characterSet = characterSet_;
		rectangular = rectangular_;
		lineCopy = lineCopy_;
		FixSelectionForClipboard();
	}
	void Copy(const SelectionText &other)
	{
		Copy(other.s, other.codePage, other.characterSet, other.rectangular, other.lineCopy);
	}
	const char * Data() const
	{
		return s.c_str();
	}
	size_t Length() const
	{
		return s.length();
	}
	size_t LengthWithTerminator() const
	{
		return s.length() + 1;
	}
	bool Empty() const
	{
		return s.empty();
	}
	bool IsRectangular() const
	{
		return rectangular;
	}
	bool IsLineCopy() const
	{
		return lineCopy;
	}
	int  GetCp() const
	{
		return codePage;
	}
	int  GetCharSet() const
	{
		return characterSet;
	}
private:
	void FixSelectionForClipboard()
	{
		// To avoid truncating the contents of the clipboard when pasted where the
		// clipboard contains NUL characters, replace NUL characters by spaces.
		std::replace(s.begin(), s.end(), '\0', ' ');
	}
	bool   rectangular;
	bool   lineCopy;
	int    codePage;
	int    characterSet;
	std::string s;
};

struct WrapPending {
	// The range of lines that need to be wrapped
	enum { 
		lineLarge = 0x7ffffff 
	};
	int start;      // When there are wraps pending, will be in document range
	int end;        // May be lineLarge to indicate all of document after start
	WrapPending()
	{
		start = lineLarge;
		end = lineLarge;
	}
	void Reset()
	{
		start = lineLarge;
		end = lineLarge;
	}
	void Wrapped(int line)
	{
		if(start == line)
			start++;
	}
	bool NeedsWrap() const
	{
		return start < end;
	}
	bool AddRange(int lineStart, int lineEnd)
	{
		const bool neededWrap = NeedsWrap();
		bool changed = false;
		if(start > lineStart) {
			start = lineStart;
			changed = true;
		}
		if((end < lineEnd) || !neededWrap) {
			end = lineEnd;
			changed = true;
		}
		return changed;
	}
};
//
//
//
class Editor : public EditModel, public DocWatcher {
	// Private so Editor objects can not be copied
	explicit Editor(const Editor &);
	Editor & operator = (const Editor &);
protected:      // ScintillaBase subclass needs access to much of Editor
	// On GTK+, Scintilla is a container widget holding two scroll bars
	// whereas on Windows there is just one window with both scroll bars turned on.
	Window wMain;   ///< The Scintilla parent window
	Window wMargin; ///< May be separate when using a scroll view for wMain
	// Style resources may be expensive to allocate so are cached between uses.
	// When a style attribute is changed, this cache is flushed. 
	bool   stylesValid;
	ViewStyle vs;
	int    technology;
	Point  sizeRGBAImage;
	float  scaleRGBAImage;
	MarginView marginView;
	EditView view;
	int    cursorMode;
	int    xCaretMargin;       ///< Ensure this many pixels visible on both sides of caret
	int    scrollWidth;
	int    caretSticky;
	int    marginOptions;
	int    multiPasteMode;
	int    virtualSpaceOptions;
	KeyMap kmap;
	Timer  timer;
	Timer  autoScrollTimer;
	enum {
		autoScrollDelay = 200
	};

	Idler  idler;
	Point  lastClick;
	uint   lastClickTime;
	Point  doubleClickCloseThreshold;
	int    dwellDelay;
	int    ticksToDwell;
	enum {
		selChar,
		selWord,
		selSubLine,
		selWholeLine
	} selectionType;

	Point ptMouseLast;
	enum {
		ddNone,
		ddInitial,
		ddDragging
	} inDragDrop;

	SelectionPosition posDrop;
	int    hotSpotClickPos;
	int    lastXChosen;
	int    lineAnchorPos;
	int    originalAnchorPos;
	int    wordSelectAnchorStartPos;
	int    wordSelectAnchorEndPos;
	int    wordSelectInitialCaretPos;
	int    targetStart;
	int    targetEnd;
	int    searchFlags;
	int    topLine;
	int    posTopLine;
	int    lengthForEncode;
	int    needUpdateUI;
	enum {
		notPainting,
		painting,
		paintAbandoned
	} paintState;

	PRectangle rcPaint;
	WorkNeeded workNeeded;
	int    idleStyling;
	int    modEventMask;
	SelectionText drag;
	int    caretXPolicy;
	int    caretXSlop; ///< Ensure this many pixels visible on both sides of caret
	int    caretYPolicy;
	int    caretYSlop; ///< Ensure this many lines visible on both sides of caret
	int    visiblePolicy;
	int    visibleSlop;
	int    searchAnchor;
	int    foldAutomatic;
	WrapPending wrapPending; // Wrapping support
	//bool   hasFocus;
	//bool   mouseDownCaptures;
	//bool   mouseWheelCaptures;
	//bool   horizontalScrollBarVisible;
	//bool   verticalScrollBarVisible;
	//bool   endAtLastLine;
	//bool   mouseSelectionRectangularSwitch;
	//bool   multipleSelection;
	//bool   additionalSelectionTyping;
	//bool   dwelling;
	//bool   dropWentOutside;
	//bool   paintAbandonedByStyling;
	//bool   paintingAllText;
	//bool   willRedrawAll;
	//bool   needIdleStyling;
	//bool   recordingMacro;
	//bool   convertPastes;

	enum {
		fHasFocus                        = 0x00000001,
		fMouseDownCaptures               = 0x00000002,
		fMouseWheelCaptures              = 0x00000004,
		fHorizontalScrollBarVisible      = 0x00000008,
		fVerticalScrollBarVisible        = 0x00000010,
		fEndAtLastLine                   = 0x00000020,
		fMouseSelectionRectangularSwitch = 0x00000040,
		fMultipleSelection               = 0x00000080,
		fAdditionalSelectionTyping       = 0x00000100,
		fDwelling                        = 0x00000200,
		fDropWentOutside                 = 0x00000400,
		fPaintAbandonedByStyling         = 0x00000800,
		fPaintingAllText                 = 0x00001000,
		fWillRedrawAll                   = 0x00002000,
		fNeedIdleStyling                 = 0x00004000,
		fRecordingMacro                  = 0x00008000,
		fConvertPastes                   = 0x00010000
	};
	uint32 Flags;

	Editor();
	virtual ~Editor();
	virtual void Initialise() = 0;
	virtual void Finalise();

	void InvalidateStyleData();
	void InvalidateStyleRedraw();
	void RefreshStyleData();
	void SetRepresentations();
	void DropGraphics(bool freeObjects);
	void AllocateGraphics();

	// The top left visible point in main window coordinates. Will be 0,0 except for
	// scroll views where it will be equivalent to the current scroll position.
	virtual Point GetVisibleOriginInMain() const;
	PointDocument FASTCALL DocumentPointFromView(const Point & rPtView) const;  // Convert a point from view space to document
	int TopLineOfMain() const;   // Return the line at Main's y coordinate 0
	virtual PRectangle GetClientRectangle() const;
	virtual PRectangle GetClientDrawingRectangle();
	PRectangle GetTextRectangle() const;

	virtual int LinesOnScreen() const;
	int    LinesToScroll() const;
	int    MaxScrollPos() const;
	SelectionPosition ClampPositionIntoDocument(SelectionPosition sp) const;
	Point  LocationFromPosition(SelectionPosition pos, PointEnd pe = peDefault);
	Point  LocationFromPosition(int pos, PointEnd pe = peDefault);
	int    XFromPosition(int pos);
	int    XFromPosition(SelectionPosition sp);
	SelectionPosition SPositionFromLocation(const Point & rPt, bool canReturnInvalid = false, bool charPosition = false, bool virtualSpace = true);
	int    PositionFromLocation(const Point & rPt, bool canReturnInvalid = false, bool charPosition = false);
	SelectionPosition SPositionFromLineX(int lineDoc, int x);
	int    PositionFromLineX(int line, int x);
	int    LineFromLocation(Point pt) const;
	void   SetTopLine(int topLineNew);

	virtual bool AbandonPaint();
	virtual void RedrawRect(PRectangle rc);
	virtual void DiscardOverdraw();
	virtual void Redraw();
	void   RedrawSelMargin(int line = -1, bool allAfter = false);
	PRectangle RectangleFromRange(Range r, int overlap);
	void   InvalidateRange(int start, int end);
	bool   UserVirtualSpace() const
	{
		return ((virtualSpaceOptions & SCVS_USERACCESSIBLE) != 0);
	}
	int    CurrentPosition() const;
	bool   SelectionEmpty() const;
	SelectionPosition SelectionStart();
	SelectionPosition SelectionEnd();
	void   SetRectangularRange();
	void   ThinRectangularRange();
	void   InvalidateSelection(SelectionRange newMain, bool invalidateWholeSelection = false);
	void   InvalidateWholeSelection();
	void   SetSelection(SelectionPosition currentPos_, SelectionPosition anchor_);
	void   SetSelection(int currentPos_, int anchor_);
	void   SetSelection(SelectionPosition currentPos_);
	void   SetSelection(int currentPos_);
	void   SetEmptySelection(SelectionPosition currentPos_);
	void   SetEmptySelection(int currentPos_);
	
	enum AddNumber { 
		addOne, 
		addEach 
	};

	void MultipleSelectAdd(AddNumber addNumber);
	bool RangeContainsProtected(int start, int end) const;
	bool SelectionContainsProtected();
	int MovePositionOutsideChar(int pos, int moveDir, bool checkLineEnd = true) const;
	SelectionPosition MovePositionOutsideChar(SelectionPosition pos, int moveDir, bool checkLineEnd = true) const;
	void MovedCaret(SelectionPosition newPos, SelectionPosition previousPos, bool ensureVisible);
	void MovePositionTo(SelectionPosition newPos, Selection::selTypes selt = Selection::noSel, bool ensureVisible = true);
	void MovePositionTo(int newPos, Selection::selTypes selt = Selection::noSel, bool ensureVisible = true);
	SelectionPosition MovePositionSoVisible(SelectionPosition pos, int moveDir);
	SelectionPosition MovePositionSoVisible(int pos, int moveDir);
	Point PointMainCaret();
	void SetLastXChosen();

	void ScrollTo(int line, bool moveThumb = true);
	virtual void ScrollText(int linesToMove);
	void HorizontalScrollTo(int xPos);
	void VerticalCentreCaret();
	void MoveSelectedLines(int lineDelta);
	void MoveSelectedLinesUp();
	void MoveSelectedLinesDown();
	void MoveCaretInsideView(bool ensureVisible = true);
	int DisplayFromPosition(int pos);

	struct XYScrollPosition {
		int xOffset;
		int topLine;
		XYScrollPosition(int xOffset_, int topLine_) : xOffset(xOffset_), topLine(topLine_)
		{
		}
		bool operator== (const XYScrollPosition & other) const
		{
			return (xOffset == other.xOffset) && (topLine == other.topLine);
		}
	};

	enum XYScrollOptions {
		xysUseMargin = 0x1,
		xysVertical = 0x2,
		xysHorizontal = 0x4,
		xysDefault = xysUseMargin|xysVertical|xysHorizontal
	};

	XYScrollPosition XYScrollToMakeVisible(const SelectionRange &range, const XYScrollOptions options);
	void SetXYScroll(XYScrollPosition newXY);
	void EnsureCaretVisible(bool useMargin = true, bool vert = true, bool horiz = true);
	void ScrollRange(SelectionRange range);
	void ShowCaretAtCurrentPosition();
	void DropCaret();
	void CaretSetPeriod(int period);
	void InvalidateCaret();
	virtual void NotifyCaretMove();
	virtual void UpdateSystemCaret();

	bool Wrapping() const;
	void NeedWrapping(int docLineStart = 0, int docLineEnd = WrapPending::lineLarge);
	bool WrapOneLine(Surface * surface, int lineToWrap);
	enum wrapScope {wsAll, wsVisible, wsIdle};

	bool WrapLines(enum wrapScope ws);
	void LinesJoin();
	void LinesSplit(int pixelWidth);

	void PaintSelMargin(Surface * surface, PRectangle &rc);
	void RefreshPixMaps(Surface * surfaceWindow);
	void Paint(Surface * surfaceWindow, PRectangle rcArea);
	long FormatRange(bool draw, Sci_RangeToFormat * pfr);
	int TextWidth(int style, const char * text);

	virtual void SetVerticalScrollPos() = 0;
	virtual void SetHorizontalScrollPos() = 0;
	virtual bool ModifyScrollBars(int nMax, int nPage) = 0;
	virtual void ReconfigureScrollBars();
	void SetScrollBars();
	void ChangeSize();

	void FilterSelections();
	int RealizeVirtualSpace(int position, uint virtualSpace);
	SelectionPosition RealizeVirtualSpace(const SelectionPosition &position);
	void AddChar(char ch);
	virtual void AddCharUTF(const char * s, uint len, bool treatAsDBCS = false);
	void ClearBeforeTentativeStart();
	void InsertPaste(const char * text, int len);
	
	enum PasteShape { 
		pasteStream = 0, 
		pasteRectangular = 1, 
		pasteLine = 2 
	};

	void InsertPasteShape(const char * text, int len, PasteShape shape);
	void FASTCALL ClearSelection(bool retainMultipleSelections = false);
	void ClearAll();
	void ClearDocumentStyle();
	void Cut();
	void PasteRectangular(SelectionPosition pos, const char * ptr, int len);
	virtual void Copy() = 0;
	virtual void CopyAllowLine();
	virtual bool CanPaste();
	virtual void Paste() = 0;
	void Clear();
	void SelectAll();
	void Undo();
	void Redo();
	void DelCharBack(bool allowLineStartDeletion);
	virtual void ClaimSelection() = 0;

	static int ModifierFlags(bool shift, bool ctrl, bool alt, bool meta = false, bool super = false);
	virtual void NotifyChange() = 0;
	virtual void NotifyFocus(bool focus);
	virtual void SetCtrlID(int identifier);
	virtual int GetCtrlID()
	{
		return ctrlID;
	}

	virtual void NotifyParent(SCNotification scn) = 0;
	virtual void NotifyStyleToNeeded(int endStyleNeeded);
	void NotifyChar(int ch);
	void NotifySavePoint(bool isSavePoint);
	void NotifyModifyAttempt();
	virtual void NotifyDoubleClick(Point pt, int modifiers);
	virtual void NotifyDoubleClick(Point pt, bool shift, bool ctrl, bool alt);
	void NotifyHotSpotClicked(int position, int modifiers);
	void NotifyHotSpotClicked(int position, bool shift, bool ctrl, bool alt);
	void NotifyHotSpotDoubleClicked(int position, int modifiers);
	void NotifyHotSpotDoubleClicked(int position, bool shift, bool ctrl, bool alt);
	void NotifyHotSpotReleaseClick(int position, int modifiers);
	void NotifyHotSpotReleaseClick(int position, bool shift, bool ctrl, bool alt);
	bool NotifyUpdateUI();
	void NotifyPainted();
	void NotifyIndicatorClick(bool click, int position, int modifiers);
	void NotifyIndicatorClick(bool click, int position, bool shift, bool ctrl, bool alt);
	bool NotifyMarginClick(Point pt, int modifiers);
	bool NotifyMarginClick(Point pt, bool shift, bool ctrl, bool alt);
	bool NotifyMarginRightClick(Point pt, int modifiers);
	void NotifyNeedShown(int pos, int len);
	void NotifyDwelling(Point pt, bool state);
	void NotifyZoom();

	void NotifyModifyAttempt(Document * document, void * userData);
	void NotifySavePoint(Document * document, void * userData, bool atSavePoint);
	void CheckModificationForWrap(DocModification mh);
	void NotifyModified(Document * document, DocModification mh, void * userData);
	void NotifyDeleted(Document * document, void * userData);
	void NotifyStyleNeeded(Document * doc, void * userData, int endPos);
	void NotifyLexerChanged(Document * doc, void * userData);
	void NotifyErrorOccurred(Document * doc, void * userData, int status);
	void NotifyMacroRecord(uint iMessage, uptr_t wParam, sptr_t lParam);

	void ContainerNeedsUpdate(int flags);
	void PageMove(int direction, Selection::selTypes selt = Selection::noSel, bool stuttered = false);
	enum { 
		cmSame, 
		cmUpper, 
		cmLower 
	};
	virtual std::string CaseMapString(const std::string &s, int caseMapping);
	void ChangeCaseOfSelection(int caseMapping);
	void LineTranspose();
	void Duplicate(bool forLine);
	virtual void CancelModes();
	void NewLine();
	SelectionPosition PositionUpOrDown(SelectionPosition spStart, int direction, int lastX);
	void CursorUpOrDown(int direction, Selection::selTypes selt);
	void ParaUpOrDown(int direction, Selection::selTypes selt);
	Range RangeDisplayLine(int lineVisible);
	int StartEndDisplayLine(int pos, bool start);
	int VCHomeDisplayPosition(int position);
	int VCHomeWrapPosition(int position);
	int LineEndWrapPosition(int position);
	int HorizontalMove(uint iMessage);
	int DelWordOrLine(uint iMessage);
	virtual int KeyCommand(uint iMessage);
	virtual int KeyDefault(int /* key */, int /*modifiers*/);
	int KeyDownWithModifiers(int key, int modifiers, bool * consumed);
	int KeyDown(int key, bool shift, bool ctrl, bool alt, bool * consumed = 0);

	void Indent(bool forwards);

	virtual CaseFolder * CaseFolderForEncoding();
	long FindText(uptr_t wParam, sptr_t lParam);
	void SearchAnchor();
	long SearchText(uint iMessage, uptr_t wParam, sptr_t lParam);
	long SearchInTarget(const char * text, int length);
	void GoToLine(int lineNo);

	virtual void CopyToClipboard(const SelectionText &selectedText) = 0;
	std::string RangeText(int start, int end) const;
	void CopySelectionRange(SelectionText * ss, bool allowLineCopy = false);
	void CopyRangeToClipboard(int start, int end);
	void CopyText(int length, const char * text);
	void SetDragPosition(SelectionPosition newPos);
	virtual void DisplayCursor(Window::Cursor c);
	virtual bool DragThreshold(Point ptStart, Point ptNow);
	virtual void StartDrag();
	void DropAt(SelectionPosition position, const char * value, size_t lengthValue, bool moving, bool rectangular);
	void DropAt(SelectionPosition position, const char * value, bool moving, bool rectangular);
	/** PositionInSelection returns true if position in selection. */
	bool PositionInSelection(int pos);
	bool PointInSelection(Point pt);
	bool PointInSelMargin(Point pt) const;
	Window::Cursor GetMarginCursor(Point pt) const;
	void TrimAndSetSelection(int currentPos_, int anchor_);
	void LineSelection(int lineCurrentPos_, int lineAnchorPos_, bool wholeLine);
	void WordSelection(int pos);
	void DwellEnd(bool mouseMoved);
	void MouseLeave();
	virtual void ButtonDownWithModifiers(Point pt, uint curTime, int modifiers);
	virtual void RightButtonDownWithModifiers(Point pt, uint curTime, int modifiers);
	virtual void ButtonDown(Point pt, uint curTime, bool shift, bool ctrl, bool alt);
	void ButtonMoveWithModifiers(Point pt, int modifiers);
	void ButtonMove(Point pt);
	void ButtonUp(Point pt, uint curTime, bool ctrl);

	void Tick();
	bool Idle();
	virtual void SetTicking(bool on);
	enum TickReason { tickCaret, tickScroll, tickWiden, tickDwell, tickPlatform };

	virtual void TickFor(TickReason reason);
	virtual bool FineTickerAvailable();
	virtual bool FineTickerRunning(TickReason reason);
	virtual void FineTickerStart(TickReason reason, int millis, int tolerance);
	virtual void FineTickerCancel(TickReason reason);
	virtual bool SetIdle(bool)
	{
		return false;
	}

	virtual void SetMouseCapture(bool on) = 0;
	virtual bool HaveMouseCapture() = 0;
	void SetFocusState(bool focusState);

	int PositionAfterArea(PRectangle rcArea) const;
	void StyleToPositionInView(Position pos);
	int PositionAfterMaxStyling(int posMax, bool scrolling) const;
	void StartIdleStyling(bool truncatedLastStyling);
	void StyleAreaBounded(PRectangle rcArea, bool scrolling);
	void IdleStyling();
	virtual void IdleWork();
	virtual void QueueIdleWork(WorkNeeded::workItems items, int upTo = 0);

	virtual bool PaintContains(PRectangle rc);
	bool PaintContainsMargin();
	void CheckForChangeOutsidePaint(Range r);
	void SetBraceHighlight(Position pos0, Position pos1, int matchStyle);

	void SetAnnotationHeights(int start, int end);
	virtual void SetDocPointer(Document * document);

	void SetAnnotationVisible(int visible);

	int ExpandLine(int line);
	void SetFoldExpanded(int lineDoc, bool expanded);
	void FoldLine(int line, int action);
	void FoldExpand(int line, int action, int level);
	int ContractedFoldNext(int lineStart) const;
	void EnsureLineVisible(int lineDoc, bool enforcePolicy);
	void FoldChanged(int line, int levelNow, int levelPrev);
	void NeedShown(int pos, int len);
	void FoldAll(int action);

	int GetTag(char * tagValue, int tagNumber);
	int ReplaceTarget(bool replacePatterns, const char * text, int length = -1);

	bool PositionIsHotspot(int position) const;
	bool PointIsHotspot(Point pt);
	void SetHotSpotRange(Point * pt);
	Range GetHotSpotRange() const;
	void SetHoverIndicatorPosition(int position);
	void SetHoverIndicatorPoint(Point pt);

	int CodePage() const;
	virtual bool ValidCodePage(int /* codePage */) const
	{
		return true;
	}

	int WrapCount(int line);
	void AddStyledText(char * buffer, int appendLength);

	virtual sptr_t DefWndProc(uint iMessage, uptr_t wParam, sptr_t lParam) = 0;
	bool ValidMargin(uptr_t wParam) const;
	void StyleSetMessage(uint iMessage, uptr_t wParam, sptr_t lParam);
	sptr_t StyleGetMessage(uint iMessage, uptr_t wParam, sptr_t lParam);
	void SetSelectionNMessage(uint iMessage, uptr_t wParam, sptr_t lParam);

	static const char * StringFromEOLMode(int eolMode);

	static sptr_t StringResult(sptr_t lParam, const char * val);
	static sptr_t BytesResult(sptr_t lParam, const uchar * val, size_t len);

public:
	// Public so the COM thunks can access it.
	bool   IsUnicodeMode() const;
	// Public so scintilla_send_message can use it.
	virtual sptr_t WndProc(uint iMessage, uptr_t wParam, sptr_t lParam);
	int    ctrlID;      // Public so scintilla_set_id can use it.
	int    errorStatus; // Public so COM methods for drag and drop can set it.
	friend class AutoSurface;
	friend class SelectionLineIterator;
};
/**
 * A smart pointer class to ensure Surfaces are set up and deleted correctly.
 */
class AutoSurface {
private:
	Surface * surf;
public:
	AutoSurface(Editor * ed, int technology = -1) : surf(0)
	{
		if(ed->wMain.GetID()) {
			surf = Surface::Allocate(technology != -1 ? technology : ed->technology);
			if(surf) {
				surf->Init(ed->wMain.GetID());
				surf->SetUnicodeMode(SC_CP_UTF8 == ed->CodePage());
				surf->SetDBCSMode(ed->CodePage());
			}
		}
	}
	AutoSurface(SurfaceID sid, Editor * ed, int technology = -1) : surf(0)
	{
		if(ed->wMain.GetID()) {
			surf = Surface::Allocate(technology != -1 ? technology : ed->technology);
			if(surf) {
				surf->Init(sid, ed->wMain.GetID());
				surf->SetUnicodeMode(SC_CP_UTF8 == ed->CodePage());
				surf->SetDBCSMode(ed->CodePage());
			}
		}
	}
	~AutoSurface()
	{
		delete surf;
	}
	Surface * operator->() const
	{
		return surf;
	}
	operator Surface *() const 
	{
		return surf;
	}
};
//
//
//
#ifdef SCI_LEXER
	class LexState;
#endif
//
//
//
class ScintillaBase : public Editor {
	// Private so ScintillaBase objects can not be copied
	explicit ScintillaBase(const ScintillaBase &);
	ScintillaBase &operator=(const ScintillaBase &);
protected:
	/** Enumeration of commands and child windows. */
	enum {
		idCallTip=1,
		idAutoComplete=2,

		idcmdUndo=10,
		idcmdRedo=11,
		idcmdCut=12,
		idcmdCopy=13,
		idcmdPaste=14,
		idcmdDelete=15,
		idcmdSelectAll=16
	};

	enum { maxLenInputIME = 200 };

	int displayPopupMenu;
	Menu popup;
	AutoComplete ac;
	CallTip ct;

	int listType;			///< 0 is an autocomplete list
	int maxListWidth;		/// Maximum width of list, in average character widths
	int multiAutoCMode; /// Mode for autocompleting when multiple selections are present
#ifdef SCI_LEXER
	LexState *DocumentLexState();
	void SetLexer(uptr_t wParam);
	void SetLexerLanguage(const char *languageName);
	void Colourise(int start, int end);
#endif
	ScintillaBase();
	virtual ~ScintillaBase();
	virtual void Initialise() = 0;
	virtual void Finalise();

	virtual void AddCharUTF(const char *s, uint len, bool treatAsDBCS=false);
	void Command(int cmdId);
	virtual void CancelModes();
	virtual int KeyCommand(uint iMessage);

	void AutoCompleteInsert(Position startPos, int removeLen, const char *text, int textLen);
	void AutoCompleteStart(int lenEntered, const char *list);
	void AutoCompleteCancel();
	void AutoCompleteMove(int delta);
	int AutoCompleteGetCurrent() const;
	int AutoCompleteGetCurrentText(char *buffer) const;
	void AutoCompleteCharacterAdded(char ch);
	void AutoCompleteCharacterDeleted();
	void AutoCompleteCompleted(char ch, uint completionMethod);
	void AutoCompleteMoveToCurrentWord();
	static void AutoCompleteDoubleClick(void *p);

	void CallTipClick();
	void CallTipShow(Point pt, const char *defn);
	virtual void CreateCallTipWindow(PRectangle rc) = 0;

	virtual void AddToPopUp(const char *label, int cmd=0, bool enabled=true) = 0;
	bool ShouldDisplayPopup(Point ptInWindowCoordinates) const;
	void ContextMenu(Point pt);

	virtual void ButtonDownWithModifiers(Point pt, uint curTime, int modifiers);
	virtual void ButtonDown(Point pt, uint curTime, bool shift, bool ctrl, bool alt);
	virtual void RightButtonDownWithModifiers(Point pt, uint curTime, int modifiers);

	void NotifyStyleToNeeded(int endStyleNeeded);
	void NotifyLexerChanged(Document *doc, void *userData);

public:
	// Public so scintilla_send_message can use it
	virtual sptr_t WndProc(uint iMessage, uptr_t wParam, sptr_t lParam);
};
//
// SparseVector is similar to RunStyles but is more efficient for cases where values occur
// for one position instead of over a range of positions.
//
template <typename T> class SparseVector {
private:
	Partitioning * starts;
	SplitVector<T> * values;
	// Private so SparseVector objects can not be copied
	SparseVector(const SparseVector &);
	void ClearValue(int partition)
	{
		values->SetValueAt(partition, T());
	}
	void CommonSetValueAt(int position, T value)
	{
		// Do the work of setting the value to allow for specialization of SetValueAt.
		assert(position < Length());
		const int partition = starts->PartitionFromPosition(position);
		const int startPartition = starts->PositionFromPartition(partition);
		if(value == T()) {
			// Setting the empty value is equivalent to deleting the position
			if(position == 0) {
				ClearValue(partition);
			}
			else if(position == startPartition) {
				// Currently an element at this position, so remove
				ClearValue(partition);
				starts->RemovePartition(partition);
				values->Delete(partition);
			}
			// Else element remains empty
		}
		else {
			if(position == startPartition) {
				// Already a value at this position, so replace
				ClearValue(partition);
				values->SetValueAt(partition, value);
			}
			else {
				// Insert a new element
				starts->InsertPartition(partition + 1, position);
				values->InsertValue(partition + 1, 1, value);
			}
		}
	}
public:
	SparseVector()
	{
		starts = new Partitioning(8);
		values = new SplitVector<T>();
		values->InsertValue(0, 2, T());
	}
	~SparseVector()
	{
		ZDELETE(starts);
		// starts dead here but not used by ClearValue.
		for(int part = 0; part < values->Length(); part++) {
			ClearValue(part);
		}
		ZDELETE(values);
	}
	int Length() const
	{
		return starts->PositionFromPartition(starts->Partitions());
	}
	int Elements() const
	{
		return starts->Partitions();
	}
	int PositionOfElement(int element) const
	{
		return starts->PositionFromPartition(element);
	}
	T ValueAt(int position) const
	{
		assert(position < Length());
		const int partition = starts->PartitionFromPosition(position);
		const int startPartition = starts->PositionFromPartition(partition);
		if(startPartition == position) {
			return values->ValueAt(partition);
		}
		else {
			return T();
		}
	}
	void SetValueAt(int position, T value)
	{
		CommonSetValueAt(position, value);
	}
	void InsertSpace(int position, int insertLength)
	{
		assert(position <= Length());   // Only operation that works at end.
		const int partition = starts->PartitionFromPosition(position);
		const int startPartition = starts->PositionFromPartition(partition);
		if(startPartition == position) {
			T valueCurrent = values->ValueAt(partition);
			// Inserting at start of run so make previous longer
			if(partition == 0) {
				// Inserting at start of document so ensure 0
				if(valueCurrent != T()) {
					ClearValue(0);
					starts->InsertPartition(1, 0);
					values->InsertValue(1, 1, valueCurrent);
					starts->InsertText(0, insertLength);
				}
				else {
					starts->InsertText(partition, insertLength);
				}
			}
			else {
				if(valueCurrent != T()) {
					starts->InsertText(partition - 1, insertLength);
				}
				else {
					// Insert at end of run so do not extend style
					starts->InsertText(partition, insertLength);
				}
			}
		}
		else {
			starts->InsertText(partition, insertLength);
		}
	}
	void DeletePosition(int position)
	{
		assert(position < Length());
		int partition = starts->PartitionFromPosition(position);
		const int startPartition = starts->PositionFromPartition(partition);
		if(startPartition == position) {
			if(partition == 0) {
				ClearValue(0);
			}
			else if(partition == starts->Partitions()) {
				// This should not be possible
				ClearValue(partition);
				throw std::runtime_error("SparseVector: deleting end partition.");
			}
			else {
				ClearValue(partition);
				starts->RemovePartition(partition);
				values->Delete(partition);
				// Its the previous partition now that gets smaller
				partition--;
			}
		}
		starts->InsertText(partition, -1);
	}
	void Check() const
	{
		if(Length() < 0) {
			throw std::runtime_error("SparseVector: Length can not be negative.");
		}
		if(starts->Partitions() < 1) {
			throw std::runtime_error("SparseVector: Must always have 1 or more partitions.");
		}
		if(starts->Partitions() != values->Length() - 1) {
			throw std::runtime_error("SparseVector: Partitions and values different lengths.");
		}
		// The final element can not be set
		if(values->ValueAt(values->Length() - 1) != T()) {
			throw std::runtime_error("SparseVector: Unused style at end changed.");
		}
	}
};

// The specialization for const char * makes copies and deletes them as needed.

template <> inline void SparseVector<const char *>::ClearValue(int partition)
{
	const char * value = values->ValueAt(partition);
	delete []value;
	values->SetValueAt(partition, 0);
}

template <> inline void SparseVector<const char *>::SetValueAt(int position, const char * value)
{
	// Make a copy of the string
	if(value) {
		const size_t len = strlen(value);
		char * valueCopy = new char[len + 1]();
		std::copy(value, value + len, valueCopy);
		CommonSetValueAt(position, valueCopy);
	}
	else {
		CommonSetValueAt(position, 0);
	}
}
// 
// The following defines are not meant to be changeable. They are for readability only.
// 
#define MAXCHR	256
#define CHRBIT	8
#define BITBLK	MAXCHR/CHRBIT

class CharacterIndexer {
public:
	virtual char CharAt(int index) = 0;
	virtual ~CharacterIndexer() 
	{
	}
};

class RESearch {
public:
	explicit RESearch(CharClassify *charClassTable);
	~RESearch();
	void Clear();
	void GrabMatches(CharacterIndexer &ci);
	const char *Compile(const char *pattern, int length, bool caseSensitive, bool posix);
	int Execute(CharacterIndexer &ci, int lp, int endp);

	enum { MAXTAG=10 };
	enum { MAXNFA=4096 };
	enum { NOTFOUND=-1 };

	int bopat[MAXTAG];
	int eopat[MAXTAG];
	std::string pat[MAXTAG];
private:
	void ChSet(uchar c);
	void ChSetWithCase(uchar c, bool caseSensitive);
	int GetBackslashExpression(const char *pattern, int &incr);
	int PMatch(CharacterIndexer &ci, int lp, int endp, char *ap);

	int bol;
	int tagstk[MAXTAG];  /* subpat tag stack */
	char nfa[MAXNFA];    /* automaton */
	int sta;
	uchar bittab[BITBLK]; /* bit table for CCL pre-set bits */
	int failure;
	CharClassify *charClass;
	bool iswordc(uchar x) const 
	{
		return charClass->IsWord(x);
	}
};
//
//
//
typedef void*(EXT_LEXER_DECL *GetLexerFunction)(uint Index);
typedef int (EXT_LEXER_DECL *GetLexerCountFn)();
typedef void (EXT_LEXER_DECL *GetLexerNameFn)(uint Index, char *name, int buflength);
typedef LexerFactoryFunction(EXT_LEXER_DECL *GetLexerFactoryFunction)(uint Index);

/// Sub-class of LexerModule to use an external lexer.
class ExternalLexerModule : public LexerModule {
protected:
	GetLexerFactoryFunction fneFactory;
	std::string name;
public:
	ExternalLexerModule(int language_, LexerFunction fnLexer_, const char *languageName_=0, LexerFunction fnFolder_=0) :
		LexerModule(language_, fnLexer_, 0, fnFolder_),
		fneFactory(0), name(languageName_)
	{
		languageName = name.c_str();
	}
	virtual void SetExternal(GetLexerFactoryFunction fFactory, int index);
};

/// LexerMinder points to an ExternalLexerModule - so we don't leak them.
class LexerMinder {
public:
	ExternalLexerModule *self;
	LexerMinder *next;
};

/// LexerLibrary exists for every External Lexer DLL, contains LexerMinders.
class LexerLibrary {
	DynamicLibrary	*lib;
	LexerMinder		*first;
	LexerMinder		*last;

public:
	explicit LexerLibrary(const char *ModuleName);
	~LexerLibrary();
	void Release();

	LexerLibrary * next;
	std::string m_sModuleName;
};

/// LexerManager manages external lexers, contains LexerLibrarys.
class LexerManager {
public:
	~LexerManager();
	static LexerManager *GetInstance();
	static void DeleteInstance();
	void Load(const char *path);
	void Clear();
private:
	LexerManager();
	static LexerManager *theInstance;
	void LoadLexerLibrary(const char *module);
	LexerLibrary *first;
	LexerLibrary *last;
};

class LMMinder {
public:
	~LMMinder();
};

#ifdef SCI_NAMESPACE
}
#endif
#endif // __SCINTILLA_INTERNAL_H