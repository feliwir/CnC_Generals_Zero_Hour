/**
    This file defines a the actionscript "Date" object handlers.
*/

#pragma once

#include "AptObject/AptObject.h"

class AptDate : public AptObject
{
  public:
#if defined(APT_USE_DATE_OBJECT)

    APT_VALUE_GC_NEW_DELETE_OPERATORS

    AptSysClock mTM;
    AptSysClock mTMU;
    int hourDiff;

    AptDate(int year, int month, int date, int hour, int minute, int second, int millisecond) : AptObject(AptVFT_Date)
    {
        hourDiff = 0;
        {
            AptGetUserFuncs().pfnGetRealTimeClock(&mTM, true);
        }
        {
            AptGetUserFuncs().pfnGetRealTimeClock(&mTMU, false);
        }
        if (mTM.Date > mTMU.Date || ((mTM.Date == mTMU.Date) && (mTM.Hour > mTMU.Hour)))
        {
            if (mTM.Date > mTMU.Date)
                hourDiff = 24 - mTMU.Hour + mTM.Hour;
            else
                hourDiff = mTM.Hour - mTMU.Hour;
        }
        else if (mTM.Date < mTMU.Date || ((mTM.Date == mTMU.Date) && (mTM.Hour < mTMU.Hour)))
        {
            if (mTM.Date < mTMU.Date)
                hourDiff = mTM.Hour - (24 + mTMU.Hour);
            else
                hourDiff = mTM.Hour - mTMU.Hour;
        }

        mTM.Year       = year == -1 ? mTM.Year : year;
        mTM.Month      = month == -1 ? mTM.Month : month;
        mTM.Date       = date == -1 ? mTM.Date : date;
        mTM.Hour       = hour == -1 ? mTM.Hour : hour;
        mTM.Minute     = minute == -1 ? mTM.Minute : minute;
        mTM.Second     = second == -1 ? mTM.Second : second;
        mTM.Hundredths = millisecond == -1 ? mTM.Hundredths : millisecond;
        setDates(&mTM, &mTMU, hourDiff);
    }

    bool dateIsYearLeap(int nYear);
    int dateGetNumDaysInYear(int nYear);
    int dateGetNumDaysInMonth(int nMonth, int nYear);
    void setDates(AptSysClock *mTM1, AptSysClock *mTM2, int hourDiff);
    int getDayOfWeek(int year, int month, int day);
    void toString(AptNativeString &sDate);
    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;

    NATIVE_MEMBER_FUNCTION_DECL(getDate);
    NATIVE_MEMBER_FUNCTION_DECL(getDay);
    NATIVE_MEMBER_FUNCTION_DECL(getFullYear);
    NATIVE_MEMBER_FUNCTION_DECL(getHours);
    NATIVE_MEMBER_FUNCTION_DECL(getMilliseconds);
    NATIVE_MEMBER_FUNCTION_DECL(getMinutes);
    NATIVE_MEMBER_FUNCTION_DECL(getMonth);
    NATIVE_MEMBER_FUNCTION_DECL(getSeconds);
    NATIVE_MEMBER_FUNCTION_DECL(getTime);
    NATIVE_MEMBER_FUNCTION_DECL(getTimezoneOffset);
    NATIVE_MEMBER_FUNCTION_DECL(getUTCDate);
    NATIVE_MEMBER_FUNCTION_DECL(getUTCDay);
    NATIVE_MEMBER_FUNCTION_DECL(getUTCFullYear);
    NATIVE_MEMBER_FUNCTION_DECL(getUTCHours);
    NATIVE_MEMBER_FUNCTION_DECL(getUTCMilliseconds);
    NATIVE_MEMBER_FUNCTION_DECL(getUTCMinutes);
    NATIVE_MEMBER_FUNCTION_DECL(getUTCMonth);
    NATIVE_MEMBER_FUNCTION_DECL(getUTCSeconds);
    NATIVE_MEMBER_FUNCTION_DECL(getYear);
    NATIVE_MEMBER_FUNCTION_DECL(setDate);
    NATIVE_MEMBER_FUNCTION_DECL(setFullYear);
    NATIVE_MEMBER_FUNCTION_DECL(setHours);
    NATIVE_MEMBER_FUNCTION_DECL(setMilliseconds);
    NATIVE_MEMBER_FUNCTION_DECL(setMinutes);
    NATIVE_MEMBER_FUNCTION_DECL(setMonth);
    NATIVE_MEMBER_FUNCTION_DECL(setSeconds);
    NATIVE_MEMBER_FUNCTION_DECL(setTime);
    NATIVE_MEMBER_FUNCTION_DECL(setUTCDate);
    NATIVE_MEMBER_FUNCTION_DECL(setUTCFullYear);
    NATIVE_MEMBER_FUNCTION_DECL(setUTCHours);
    NATIVE_MEMBER_FUNCTION_DECL(setUTCMilliseconds);
    NATIVE_MEMBER_FUNCTION_DECL(setUTCMinutes);
    NATIVE_MEMBER_FUNCTION_DECL(setUTCMonth);
    NATIVE_MEMBER_FUNCTION_DECL(setUTCSeconds);
    NATIVE_MEMBER_FUNCTION_DECL(setYear);
    NATIVE_MEMBER_FUNCTION_DECL(toString);
    NATIVE_MEMBER_FUNCTION_DECL(UTC);
    static void CleanNativeFunctions();
#else
    AptDate(int year, int month, int date, int hour, int minute, int second, int millisecond) : AptObject(AptVFT_Date)
    {
        APT_ASSERT(0 && "APT-Warning-Date object cannot be used as it is compiled out, make sure to compile with APT_USE_DATE_OBJECT defined");
    }

    static void *operator new(size_t size)
    {
        APT_ASSERT(0 && "APT-Warning-Date object cannot be used as it is compiled out, make sure to compile with APT_USE_DATE_OBJECT defined");
        return (void *)0x00000000;
    }
    static void operator delete(void *p, size_t size) { APT_ASSERT(0 && "APT-Warning-Date object cannot be used as it is compiled out, make sure to compile with APT_USE_DATE_OBJECT defined"); }
    static void *operator new[](size_t)
    {
        APT_ASSERT(0 && "APT-Warning-Date object cannot be used as it is compiled out, make sure to compile with APT_USE_DATE_OBJECT defined");
        return (void *)0x00000000;
    }
    static void operator delete[](void *) { APT_ASSERT(0 && "APT-Warning-Date object cannot be used as it is compiled out, make sure to compile with APT_USE_DATE_OBJECT defined"); }
#endif

  protected:
    APT_INLINE
    virtual ~AptDate()
    {
        //  Do nothing...
    }
};
