//****************************************************************************
// ActionScript Standard Library
// flash.filters.DropShadowFilter object
//
// Note: Differences between Flash and Apt behavior
//
//       The native Flash version of this class does range-checking on the value
//       passed by the user when one of the filter properties are set.
//       In Apt, we only do range-checking when the property value is passed
//       into the constructor, and NOT when the property is set directly.
//       For example:
//
//         // passing 720 degress for the angle param
//         var f  = new flash.filters.DropShadowFilter( 100, 720 );
//         trace( f.angle );  // Both Flash and Apt return 360
//         f.angle = 720;
//         trace( f.angle );  // Flash returns 360, Apt returns 720
//
//       Another difference between Apt and Flash is that the "quality"
//       property, below, is always an integer in Flash. In Apt is is a float.
//
//****************************************************************************

class flash.filters.DropShadowFilter extends flash.filters.BitmapFilter
{
    var distance:Number;
    var angle:Number;
    var color:Number;
    var alpha:Number;
    var blurX:Number;
    var blurY:Number;
    var strength:Number;
    var quality:Number;
    var inner:Boolean;
    var knockout:Boolean;
    var hideObject:Boolean;

    public function DropShadowFilter(
        distance:Number,
        angle:Number,
        color:Number,
        alpha:Number,
        blurX:Number,
        blurY:Number,
        strength:Number,
        quality:Number,
        inner:Boolean,
        knockout:Boolean,
        hideObject:Boolean
    )
    {
        filterType = 0; // same as Flash 8 spec

        if ( distance==undefined )
            this.distance=4.0;
        else
            this.distance=distance;
        if ( angle==undefined )
            this.angle=45.0;
        else
            this.angle = angle % 360;
        if ( color==undefined )
            this.color=0x000000;
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
        if ( strength==undefined )
            this.strength=1;
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
        if ( hideObject==undefined )
            this.hideObject=false;
        else
            this.hideObject=hideObject;
    }


    public function clone():DropShadowFilter
    {
        return new DropShadowFilter( distance,
                                     angle,
                                     color,
                                     alpha,
                                     blurX,
                                     blurY,
                                     strength,
                                     quality,
                                     inner,
                                     knockout,
                                     hideObject );
    }
}


