//****************************************************************************
// ActionScript Standard Library
// flash.filters.GlowFilter object
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
//         var f  = new flash.filters.GlowFilter( 0xFFFF, 0.25, 1000 );
//         trace( f.blurX );  // Both Flash and Apt return 255
//         f.blurX = 1000;
//         trace( f.blurX );  // Flash returns 255, Apt returns 1000
//
//       Another difference between Apt and Flash is that the "quality"
//       property, below, is always an integer in Flash. In Apt is is a float.
//
//****************************************************************************

class flash.filters.GlowFilter extends flash.filters.BitmapFilter
{
    var color:Number;
    var alpha:Number;
    var blurX:Number;
    var blurY:Number;
    var strength:Number;
    var quality:Number;
    var inner:Boolean;
    var knockout:Boolean;

    public function GlowFilter(
        color:Number,
        alpha:Number,
        blurX:Number,
        blurY:Number,
        strength:Number,
        quality:Number,
        inner:Boolean,
        knockout:Boolean
    )
    {
        filterType = 2;     // same as Flash 8 spec

        if ( color==undefined )
            this.color=0xFF0000;
        else
            this.color = color & 0xFFFFFF;
        if ( alpha==undefined )
            this.alpha=1.0;
        else
        {
            if ( alpha > 1 ) alpha = 1;
            else if ( alpha < 0 ) alpha = 0;
            this.alpha=alpha;
        }
        if ( blurX==undefined )
            this.blurX=6.0;
        else
        {
            if ( blurX > 255 ) blurX = 255;
            else if ( blurX < 0 ) blurX = 0;
            this.blurX=blurX;
        }
        if ( blurY==undefined )
            this.blurY=6.0;
        else
        {
            if ( blurY > 255 ) blurY = 255;
            else if ( blurY < 0 ) blurY = 0;
            this.blurY=blurY;
        }
        if ( strength==undefined )
            this.strength=2;
        else
        {
            if ( strength > 255 ) strength = 255;
            else if ( strength < 0 ) strength = 0;
            this.strength=strength;
        }
        if ( quality==undefined )
            this.quality=1;
        else
        {
            if ( quality > 15 ) quality = 15;
            else if ( quality < 0 ) quality = 0;
            this.quality = quality;
        }
        if ( inner==undefined )
            this.inner=false;
        else
            this.inner=inner;
        if ( knockout==undefined )
            this.knockout=false;
        else
            this.knockout=knockout;
    }

    public function clone():GlowFilter
    {
        return new GlowFilter(
           color,
           alpha,
           blurX,
           blurY,
           strength,
           quality,
           inner,
           knockout
        );
    }
}

