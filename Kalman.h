#ifndef _Kalman_h
#define _Kalman_h

class Kalman {
public:
  float   x;          // Predicted value
  float   r;          // Sensor noise
  float   pn;         // Process noise
  float   p;          // Predicted error
  
  Kalman( float _x, float _r, float _pn, float _p ) {
    x  = _x;
    r  = _r;
    pn = _pn;
    p  = _p;
  }
  
  float predict( float f ) {
    float pc = p + pn;
    float g  = ( pc == 0 ? 1 : pc / ( pc + r ) );
    p = ( 1 - g ) * pc;
    x = g * ( f - x ) + x;
    return x;
  }
};

#endif
