//****************************************************************************
// ActionScript Standard Library
// flash.filters.ColorMatrixFilter object
//
// Note: Differences between Flash and Apt behavior
//
//       The native Flash version of this class does range-checking on the value
//       passed by the user when one of the filter properties are set.
//       In Apt, we only do range-checking when the property value is passed
//       into the constructor, and NOT when the property is set directly.
//
//****************************************************************************

class flash.filters.ColorMatrixFilter extends flash.filters.BitmapFilter
{
    var matrix:Array;

    public function ColorMatrixFilter(matrix:Array)
    {
        filterType = 6;     // same as Flash 8 spec
        if ( matrix==undefined )
        {
            this.matrix = new Array( 1,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,1,0 );
        }
        else
        {
            this.matrix = matrix;
        }
    }

    public function clone():ColorMatrixFilter
    {
        return new ColorMatrixFilter(matrix);
    }
}
