#pragma once
#include "zhuziControl.h"
#include <functional>
#include <vector>

namespace zhuzi {

    class zhuziDTP : public zhuziControl {
    public:
        enum class Format {
            ShortDate, LongDate, Time, TimeWithSeconds, DateTime, DateTimeWithSeconds, Custom
        };

        zhuziDTP(zhuziControl* parent = nullptr);
        ~zhuziDTP();

        virtual bool onCreate(DWORD style) override;

        // 样式设置
        void setFormat(Format format);
        void setCustomFormat(const zhuziString& format);
        void setUseUpDown(bool use);
        void setShowNone(bool show);
        void setCheckNone(bool checked);
        bool isNoneChecked() const;
        void setReadOnly(bool readOnly);
        void setAllowEdit(bool allow);

        // 日期/时间设置
        void setDateTime(const SYSTEMTIME& st);
        void setDateTime(WORD year, WORD month, WORD day, WORD hour = 0, WORD minute = 0, WORD second = 0);
        void setDate(WORD year, WORD month, WORD day);
        void setTime(WORD hour, WORD minute, WORD second = 0);
        SYSTEMTIME getDateTime() const;
        bool getDateTime(SYSTEMTIME& st) const;
        zhuziString getDateTimeString() const;
        void setToNow();
        void setToToday();
        void setToNone();

        // 范围限制
        void setMinRange(const SYSTEMTIME& st, bool hasMin = true);
        void setMaxRange(const SYSTEMTIME& st, bool hasMax = true);
        void clearRange();
        void setYearRange(int minYear, int maxYear);

        // 样式查询
        DWORD getDTPStyle() const;
        bool isUpDownStyle() const;
        bool isShowNoneStyle() const;

        // 事件
        void setOnDateTimeChange(std::function<void(const SYSTEMTIME&)> callback);
        void setOnCheckChange(std::function<void(bool)> callback);
        void enable(bool enabled = true) override;

        int getRecommendedWidth() const;
        static int getRecommendedHeight() { return 22; }
        void redraw();

    private:
        void setupEventHandling();
        void sendDateTimeChangeNotification();

        std::function<void(const SYSTEMTIME&)> m_onDateTimeChange;
        std::function<void(bool)> m_onCheckChange;

        bool m_hasCustomFormat;
        zhuziString m_customFormatString;
        Format m_currentFormat;
        bool m_useUpDown;
        bool m_showNone;

        struct RangeInfo {
            bool hasMin, hasMax;
            SYSTEMTIME min, max;
        } m_rangeInfo;

        static const wchar_t* DTP_CLASS_NAME;
        static bool s_classInitialized;
        static void initDTPClass();
    };

    class zhuziMonthCalendar : public zhuziControl {
    public:
        zhuziMonthCalendar(zhuziControl* parent = nullptr);
        ~zhuziMonthCalendar();

        virtual bool onCreate(DWORD style) override;

        void setDate(const SYSTEMTIME& st);
        SYSTEMTIME getDate() const;
        bool getDate(SYSTEMTIME& st) const;
        void setToday(const SYSTEMTIME& st);
        SYSTEMTIME getToday() const;
        void setToToday();
        void setMinDate(const SYSTEMTIME& minDate);
        void setMaxDate(const SYSTEMTIME& maxDate);
        void clearDateRange();
        void setTitleBackColor(COLORREF color);
        void setTitleTextColor(COLORREF color);
        void setTrailingTextColor(COLORREF color);
        void setFirstDayOfWeek(int dayOfWeek);
        void setOnSelChange(std::function<void(const SYSTEMTIME&)> callback);

    private:
        std::function<void(const SYSTEMTIME&)> m_onSelChange;
        static const wchar_t* MONTHCAL_CLASS_NAME;
        static bool s_classInitialized;
        static void initMonthCalClass();
    };
} // namespace zhuzi