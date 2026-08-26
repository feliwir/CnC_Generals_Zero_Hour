//****************************************************************************
// ActionScript Standard Library
// flash.filters.GradientBevelFilter object
//
// Note: Differences between Flash and Apt behavior
//
//       The native Flash version of this class does range-checking on the value
//       passed by the user when one of the filter properties are set.
//       In Apt, we only do range-checking when the property value is passed
//       into the constructor, and NOT when the property is set directly.
//
//****************************************************************************

class flash.filters.GradientBevelFilter extends flash.filters.BitmapFilter
{

    public function GradientBevelFilter(
    )
    {
        filterType = 7;     // same as Flash 8 spec
    }

    public function clone():GradientBevelFilter
    {
        return new GradientBevelFilter();
    }
}
