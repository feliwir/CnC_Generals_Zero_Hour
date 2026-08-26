//****************************************************************************
// ActionScript Standard Library
// flash.filters.DisplacementMapFilter object
//
// Note: Differences between Flash and Apt behavior
//
//       The native Flash version of this class does range-checking on the value
//       passed by the user when one of the filter properties are set.
//       In Apt, we only do range-checking when the property value is passed
//       into the constructor, and NOT when the property is set directly.
//
//****************************************************************************

class flash.filters.DisplacementMapFilter extends flash.filters.BitmapFilter
{
    var blurX:Number;
    var blurY:Number;
    var quality:Number;

    public function DisplacementMapFilter(
    )
    {
        filterType = 8;     // Not sure; this is not in the Flash 8 spec

    }

    public function clone():DisplacementMapFilter
    {
        return new DisplacementMapFilter( blurX, blurY, quality );
    }
}
