//****************************************************************************
// ActionScript Standard Library
// flash.filters.BlurFilter object
//
// Note: Differences between Flash and Apt behavior
//
//       The native Flash version of this class does range-checking on the value
//       passed by the user when one of the filter properties are set.
//       In Apt, we only do range-checking when the property value is passed
//       into the constructor, and NOT when the property is set directly.
//       For example:
//
//         // passing 1000 for the blurX param
//         var f  = new flash.filters.BlurFilter( 1000 );
//         trace( f.blurX );  // Both Flash and Apt return 255
//         f.blurX = 1000;
//         trace( f.blurX );  // Flash returns 255, Apt returns 1000
//
//       Another difference between Apt and Flash is that the "quality"
//       property, below, is always an integer in Flash. In Apt is is a float.
//
//****************************************************************************

class flash.filters.BlurFilter extends flash.filters.BitmapFilter
{
    var blurX:Number;
    var blurY:Number;
    var quality:Number;

    public function BlurFilter(
        blurX:Number,
        blurY:Number,
        quality:Number
    )
    {
        filterType = 1;     // same as Flash 8 spec

        if ( blurX==undefined )
            this.blurX=4.0;
        else
        {
            if ( blurX > 255 ) blurX = 255;
            else if ( blurX < 0 ) blurX = 0;
            this.blurX=blurX;
        }
        if ( blurY==undefined )
            this.blurY=4.0;
        else
        {
            if ( blurY > 255 ) blurY = 255;
            else if ( blurY < 0 ) blurY = 0;
            this.blurY=blurY;
        }
        if ( quality==undefined )
            this.quality=1;
        else
        {
            if ( quality > 15 ) quality = 15;
            else if ( quality < 0 ) quality = 0;
            this.quality = quality;
        }
    }

    public function clone():BlurFilter
    {
        return new BlurFilter( blurX, blurY, quality );
    }
}
