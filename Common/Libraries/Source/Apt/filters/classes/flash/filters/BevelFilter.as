//****************************************************************************
// ActionScript Standard Library
// flash.filters.BevelFilter object
//
// Note: Differences between Flash and Apt behavior
//
//       The native Flash version of this class does range-checking on the value
//       passed by the user when one of the filter properties are set.
//       In Apt, we only do range-checking when the property value is passed
//       into the constructor, and NOT when the property is set directly.
//
//****************************************************************************

class flash.filters.BevelFilter extends flash.filters.BitmapFilter
{
    public function BevelFilter(
    )
    {
        filterType = 3;     // same as Flash 8 spec

    }

    public function clone():BevelFilter
    {
        return new BevelFilter();
    }
}
