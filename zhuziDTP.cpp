#include "zhuziDTP.h"
#include "zhuziInstance.h"
#include "zhuziControl.h"
#include <commctrl.h>

namespace zhuzi {

    const wchar_t* zhuziDTP::DTP_CLASS_NAME = DATETIMEPICK_CLASSW;
    bool zhuziDTP::s_classInitialized = false;
    const wchar_t* zhuziMonthCalendar::MONTHCAL_CLASS_NAME = MONTHCAL_CLASSW;
    bool zhuziMonthCalendar::s_classInitialized = false;

    void zhuziDTP::initDTPClass() {
        if (!s_classInitialized) {
            INITCOMMONCONTROLSEX icex = {};
            icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            icex.dwICC = ICC_DATE_CLASSES;
            InitCommonControlsEx(&icex);
            s_classInitialized = true;
        }
    }

    zhuziDTP::zhuziDTP(zhuziControl* parent)
        : zhuziControl(parent)
        , m_hasCustomFormat(false)
        , m_currentFormat(Format::ShortDate)
        , m_useUpDown(false)
        , m_showNone(false)
        , m_eventBound(false) {
        m_rangeInfo.hasMin = false;
        m_rangeInfo.hasMax = false;
        ZeroMemory(&m_rangeInfo.min, sizeof(SYSTEMTIME));
        ZeroMemory(&m_rangeInfo.max, sizeof(SYSTEMTIME));
        initDTPClass();
    }

    zhuziDTP::~zhuziDTP() { destroy(); }

    bool zhuziDTP::onCreate(DWORD style) {
        DWORD dtpStyle = WS_CHILD | WS_VISIBLE | style;
        if (m_useUpDown) dtpStyle |= DTS_UPDOWN;
        if (m_showNone) dtpStyle |= DTS_SHOWNONE;
        dtpStyle |= DTS_SHORTDATEFORMAT; // 默认短日期

        if (!createControl(DTP_CLASS_NAME, 0, 0, 0, 0, dtpStyle))
            return false;

        // 后续初始化
        if (m_hasCustomFormat && !m_customFormatString.empty())
            DateTime_SetFormat(m_hwnd, m_customFormatString.c_str());
        else
            setFormat(m_currentFormat);

        if (m_rangeInfo.hasMin || m_rangeInfo.hasMax) {
            SYSTEMTIME* pMin = m_rangeInfo.hasMin ? &m_rangeInfo.min : nullptr;
            SYSTEMTIME* pMax = m_rangeInfo.hasMax ? &m_rangeInfo.max : nullptr;
            SendMessageW(m_hwnd, DTM_SETRANGE,
                (m_rangeInfo.hasMin ? GDTR_MIN : 0) | (m_rangeInfo.hasMax ? GDTR_MAX : 0),
                (LPARAM)MAKELPARAM(pMin ? (DWORD_PTR)pMin : 0, pMax ? (DWORD_PTR)pMax : 0));
        }

        setToNow();
        setupEventHandling();
        return true;
    }

    void zhuziDTP::setFormat(Format format) {
        m_currentFormat = format;
        m_hasCustomFormat = false;
        if (!m_hwnd) return;

        DWORD currentStyle = (DWORD)GetWindowLongPtrW(m_hwnd, GWL_STYLE);
        currentStyle &= ~(DTS_SHORTDATEFORMAT | DTS_LONGDATEFORMAT | DTS_TIMEFORMAT);
        const wchar_t* formatString = nullptr;

        switch (format) {
        case Format::ShortDate: currentStyle |= DTS_SHORTDATEFORMAT; break;
        case Format::LongDate:  currentStyle |= DTS_LONGDATEFORMAT; break;
        case Format::Time:      currentStyle |= DTS_TIMEFORMAT; break;
        case Format::TimeWithSeconds: formatString = L"HH:mm:ss"; break;
        case Format::DateTime:  formatString = L"yyyy/MM/dd HH:mm"; break;
        case Format::DateTimeWithSeconds: formatString = L"yyyy/MM/dd HH:mm:ss"; break;
        case Format::Custom:    if (m_hasCustomFormat) formatString = m_customFormatString.c_str(); break;
        }

        SetWindowLongPtrW(m_hwnd, GWL_STYLE, currentStyle);
        if (formatString) DateTime_SetFormat(m_hwnd, formatString);
        else if (format == Format::ShortDate || format == Format::LongDate || format == Format::Time)
            DateTime_SetFormat(m_hwnd, nullptr);

        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void zhuziDTP::setCustomFormat(const zhuziString& format) {
        m_hasCustomFormat = true;
        m_customFormatString = format;
        m_currentFormat = Format::Custom;
        if (m_hwnd) {
            DWORD style = (DWORD)GetWindowLongPtrW(m_hwnd, GWL_STYLE);
            style &= ~(DTS_SHORTDATEFORMAT | DTS_LONGDATEFORMAT | DTS_TIMEFORMAT);
            SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
            DateTime_SetFormat(m_hwnd, format.c_str());
        }
    }

    void zhuziDTP::setUseUpDown(bool use) {
        m_useUpDown = use;
        if (m_hwnd) {
            DWORD style = (DWORD)GetWindowLongPtrW(m_hwnd, GWL_STYLE);
            use ? style |= DTS_UPDOWN : style &= ~DTS_UPDOWN;
            SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
            SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }

    void zhuziDTP::setShowNone(bool show) {
        m_showNone = show;
        if (m_hwnd) {
            DWORD style = (DWORD)GetWindowLongPtrW(m_hwnd, GWL_STYLE);
            show ? style |= DTS_SHOWNONE : style &= ~DTS_SHOWNONE;
            SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
            SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }

    void zhuziDTP::setCheckNone(bool checked) {
        if (m_showNone && m_hwnd)
            DateTime_SetSystemtime(m_hwnd, checked ? GDT_NONE : GDT_VALID, nullptr);
    }

    bool zhuziDTP::isNoneChecked() const {
        if (!m_showNone || !m_hwnd) return false;
        SYSTEMTIME st;
        return DateTime_GetSystemtime(m_hwnd, &st) == GDT_NONE;
    }

    void zhuziDTP::setReadOnly(bool readOnly) {
        if (m_hwnd) {
            HWND edit = GetWindow(m_hwnd, GW_CHILD);
            while (edit) {
                wchar_t className[32];
                GetClassNameW(edit, className, 32);
                if (wcscmp(className, L"Edit") == 0) {
                    SendMessageW(edit, EM_SETREADONLY, readOnly ? TRUE : FALSE, 0);
                    break;
                }
                edit = GetWindow(edit, GW_HWNDNEXT);
            }
        }
    }

    void zhuziDTP::setAllowEdit(bool allow) { setReadOnly(!allow); }

    void zhuziDTP::setDateTime(const SYSTEMTIME& st) {
        if (m_hwnd) DateTime_SetSystemtime(m_hwnd, GDT_VALID, &st);
    }

    void zhuziDTP::setDateTime(WORD year, WORD month, WORD day, WORD hour, WORD minute, WORD second) {
        SYSTEMTIME st = { year, month, 0, day, hour, minute, second, 0 };
        setDateTime(st);
    }

    void zhuziDTP::setDate(WORD year, WORD month, WORD day) {
        SYSTEMTIME st = getDateTime();
        st.wYear = year; st.wMonth = month; st.wDay = day;
        setDateTime(st);
    }

    void zhuziDTP::setTime(WORD hour, WORD minute, WORD second) {
        SYSTEMTIME st = getDateTime();
        st.wHour = hour; st.wMinute = minute; st.wSecond = second;
        setDateTime(st);
    }

    SYSTEMTIME zhuziDTP::getDateTime() const {
        SYSTEMTIME st;
        if (!getDateTime(st)) GetLocalTime(&st);
        return st;
    }

    bool zhuziDTP::getDateTime(SYSTEMTIME& st) const {
        if (!m_hwnd) return false;
        return DateTime_GetSystemtime(m_hwnd, &st) == GDT_VALID;
    }

    zhuziString zhuziDTP::getDateTimeString() const {
        if (!m_hwnd) return L"";
        int len = GetWindowTextLengthW(m_hwnd);
        if (len <= 0) return L"";
        zhuziString text(len + 1, L'\0');
        GetWindowTextW(m_hwnd, &text[0], len + 1);
        return text;
    }

    void zhuziDTP::setToNow() {
        SYSTEMTIME st; GetLocalTime(&st); setDateTime(st);
    }

    void zhuziDTP::setToToday() {
        SYSTEMTIME st; GetLocalTime(&st);
        st.wHour = st.wMinute = st.wSecond = st.wMilliseconds = 0;
        setDateTime(st);
    }

    void zhuziDTP::setToNone() {
        if (m_showNone && m_hwnd) DateTime_SetSystemtime(m_hwnd, GDT_NONE, nullptr);
    }

    void zhuziDTP::setMinRange(const SYSTEMTIME& st, bool hasMin) {
        m_rangeInfo.hasMin = hasMin;
        if (hasMin) m_rangeInfo.min = st;
        if (m_hwnd) {
            SYSTEMTIME* pMin = m_rangeInfo.hasMin ? &m_rangeInfo.min : nullptr;
            SYSTEMTIME* pMax = m_rangeInfo.hasMax ? &m_rangeInfo.max : nullptr;
            DWORD flags = (m_rangeInfo.hasMin ? GDTR_MIN : 0) | (m_rangeInfo.hasMax ? GDTR_MAX : 0);
            if (flags) SendMessageW(m_hwnd, DTM_SETRANGE, flags, (LPARAM)MAKELPARAM(pMin ? (DWORD_PTR)pMin : 0, pMax ? (DWORD_PTR)pMax : 0));
        }
    }

    void zhuziDTP::setMaxRange(const SYSTEMTIME& st, bool hasMax) {
        m_rangeInfo.hasMax = hasMax;
        if (hasMax) m_rangeInfo.max = st;
        if (m_hwnd) {
            SYSTEMTIME* pMin = m_rangeInfo.hasMin ? &m_rangeInfo.min : nullptr;
            SYSTEMTIME* pMax = m_rangeInfo.hasMax ? &m_rangeInfo.max : nullptr;
            DWORD flags = (m_rangeInfo.hasMin ? GDTR_MIN : 0) | (m_rangeInfo.hasMax ? GDTR_MAX : 0);
            if (flags) SendMessageW(m_hwnd, DTM_SETRANGE, flags, (LPARAM)MAKELPARAM(pMin ? (DWORD_PTR)pMin : 0, pMax ? (DWORD_PTR)pMax : 0));
        }
    }

    void zhuziDTP::clearRange() {
        m_rangeInfo.hasMin = m_rangeInfo.hasMax = false;
        if (m_hwnd) SendMessageW(m_hwnd, DTM_SETRANGE, 0, 0);
    }

    void zhuziDTP::setYearRange(int minYear, int maxYear) {
        SYSTEMTIME minSt, maxSt;
        GetLocalTime(&minSt); GetLocalTime(&maxSt);
        minSt.wYear = minYear; minSt.wMonth = 1; minSt.wDay = 1; minSt.wHour = minSt.wMinute = minSt.wSecond = 0;
        maxSt.wYear = maxYear; maxSt.wMonth = 12; maxSt.wDay = 31; maxSt.wHour = 23; maxSt.wMinute = 59; maxSt.wSecond = 59;
        setMinRange(minSt, true);
        setMaxRange(maxSt, true);
    }

    DWORD zhuziDTP::getDTPStyle() const { return m_hwnd ? (DWORD)GetWindowLongPtrW(m_hwnd, GWL_STYLE) : 0; }
    bool zhuziDTP::isUpDownStyle() const { return m_hwnd ? (GetWindowLongPtrW(m_hwnd, GWL_STYLE) & DTS_UPDOWN) != 0 : m_useUpDown; }
    bool zhuziDTP::isShowNoneStyle() const { return m_hwnd ? (GetWindowLongPtrW(m_hwnd, GWL_STYLE) & DTS_SHOWNONE) != 0 : m_showNone; }

    void zhuziDTP::setupEventHandling() {
        if (m_eventBound) return;          // 防止重复绑定
        if (!m_hwnd) return;
        zhuziWindow* parentWindow = findParentWindow(this);
        if (!parentWindow) return;

        m_eventBound = true;
        parentWindow->Bind(WM_NOTIFY, [this](zhuziMessage& msg) -> bool {
            NMHDR* pNmhdr = (NMHDR*)msg.lParam;
            if (pNmhdr->hwndFrom == m_hwnd) {
                if (pNmhdr->code == DTN_DATETIMECHANGE || pNmhdr->code == DTN_USERSTRING) {
                    sendDateTimeChangeNotification();
                    return true;   // 已处理
                }
            }
            return false;  // 未处理，继续传递
            });
    }

    void zhuziDTP::sendDateTimeChangeNotification() {
        if (m_onDateTimeChange) {
            SYSTEMTIME st = getDateTime();
            m_onDateTimeChange(st);
        }
        if (m_onCheckChange && m_showNone) m_onCheckChange(isNoneChecked());
    }

    void zhuziDTP::setOnDateTimeChange(std::function<void(const SYSTEMTIME&)> callback) { m_onDateTimeChange = callback; setupEventHandling(); }
    void zhuziDTP::setOnCheckChange(std::function<void(bool)> callback) { m_onCheckChange = callback; setupEventHandling(); }
    void zhuziDTP::enable(bool enabled) { if (m_hwnd) EnableWindow(m_hwnd, enabled); }
    int zhuziDTP::getRecommendedWidth() const { switch (m_currentFormat) {
    case Format::ShortDate: return 100; case Format::LongDate: return 160; case Format::Time: return 80; case Format::TimeWithSeconds: return 90; case Format::DateTime: return 140; case Format::DateTimeWithSeconds: return 150; default: return 120; 
    } 
    }

    void zhuziDTP::redraw() { if (m_hwnd) { InvalidateRect(m_hwnd, nullptr, TRUE); UpdateWindow(m_hwnd); } }

    // ==================== zhuziMonthCalendar ====================
    void zhuziMonthCalendar::initMonthCalClass() {
        if (!s_classInitialized) {
            INITCOMMONCONTROLSEX icex = {};
            icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            icex.dwICC = ICC_DATE_CLASSES;
            InitCommonControlsEx(&icex);
            s_classInitialized = true;
        }
    }

    zhuziMonthCalendar::zhuziMonthCalendar(zhuziControl* parent)
        : zhuziControl(parent), m_eventBound(false) {
        initMonthCalClass();
    }
    zhuziMonthCalendar::~zhuziMonthCalendar() { destroy(); }

    bool zhuziMonthCalendar::onCreate(DWORD style) {
        DWORD calStyle = WS_CHILD | WS_VISIBLE | style;
        if (!createControl(MONTHCAL_CLASS_NAME, 0, 0, 0, 0, calStyle)) return false;

        zhuziWindow* parentWindow = findParentWindow(this);
        if (parentWindow && !m_eventBound) {
            m_eventBound = true;
            parentWindow->Bind(WM_NOTIFY, [this](zhuziMessage& msg) -> bool {
                NMHDR* pNmhdr = (NMHDR*)msg.lParam;
                if (pNmhdr->hwndFrom == m_hwnd && pNmhdr->code == MCN_SELCHANGE) {
                    NMSELCHANGE* pChange = (NMSELCHANGE*)msg.lParam;
                    if (m_onSelChange) m_onSelChange(pChange->stSelStart);
                    return true;
                }
                return false;
                });
        }
        setToToday();
        return true;
    }

    void zhuziMonthCalendar::setDate(const SYSTEMTIME& st) { if (m_hwnd) MonthCal_SetCurSel(m_hwnd, &st); }
    SYSTEMTIME zhuziMonthCalendar::getDate() const { SYSTEMTIME st; if (!getDate(st)) GetLocalTime(&st); return st; }
    bool zhuziMonthCalendar::getDate(SYSTEMTIME& st) const { return m_hwnd ? MonthCal_GetCurSel(m_hwnd, &st) != 0 : false; }
    void zhuziMonthCalendar::setToday(const SYSTEMTIME& st) { if (m_hwnd) MonthCal_SetToday(m_hwnd, &st); }
    SYSTEMTIME zhuziMonthCalendar::getToday() const { SYSTEMTIME st; if (m_hwnd) MonthCal_GetToday(m_hwnd, &st); else GetLocalTime(&st); return st; }
    void zhuziMonthCalendar::setToToday() { SYSTEMTIME st; GetLocalTime(&st); setDate(st); setToday(st); }
    void zhuziMonthCalendar::setMinDate(const SYSTEMTIME& minDate) { if (m_hwnd) SendMessageW(m_hwnd, MCM_SETRANGE, GDTR_MIN, (LPARAM)&minDate); }
    void zhuziMonthCalendar::setMaxDate(const SYSTEMTIME& maxDate) { if (m_hwnd) SendMessageW(m_hwnd, MCM_SETRANGE, GDTR_MAX, (LPARAM)&maxDate); }
    void zhuziMonthCalendar::clearDateRange() { if (m_hwnd) SendMessageW(m_hwnd, MCM_SETRANGE, 0, 0); }
    void zhuziMonthCalendar::setTitleBackColor(COLORREF color) { if (m_hwnd) MonthCal_SetColor(m_hwnd, MCSC_TITLEBK, color); }
    void zhuziMonthCalendar::setTitleTextColor(COLORREF color) { if (m_hwnd) MonthCal_SetColor(m_hwnd, MCSC_TITLETEXT, color); }
    void zhuziMonthCalendar::setTrailingTextColor(COLORREF color) { if (m_hwnd) MonthCal_SetColor(m_hwnd, MCSC_TRAILINGTEXT, color); }
    void zhuziMonthCalendar::setFirstDayOfWeek(int dayOfWeek) { if (m_hwnd) MonthCal_SetFirstDayOfWeek(m_hwnd, dayOfWeek); }
    void zhuziMonthCalendar::setOnSelChange(std::function<void(const SYSTEMTIME&)> callback) { m_onSelChange = callback; }
} // namespace zhuzi