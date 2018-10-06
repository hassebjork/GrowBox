#ifndef _Kalman_h
#define _Kalman_h

class Kalman {
public:
  float   x;          // Predicted value
  float   r;          // Sensor noise
  float   pn;         // Process noise
  float   p;          // Predicted error
  
  Kalman() {
    Kalman( 0.0, 0.2, 0.1, 1.0 );
  }
  
  Kalman( float _x, float _r, float _pn, float _p ) {
    x  = _x;
    r  = _r;
    pn = _pn;
    p  = _p;
  }
  
//  float predict( float f ) {
//    float pc = p + pn;
//    float g  = ( pc == 0 ? 1 : pc / ( pc + r ) );
//    p = ( 1 - g ) * pc;
//    x = g * ( f - x ) + x;
//    return x;
//  }

  // https://magesblog.com/post/2014-12-02-measuring-temperature-with-my-arduino/
  float predict( float f ) {
    p = p + pn;
    float K = p / (p + r);    
    x = K * f + (1 - K) * x;
    p = (1 - K) * p;
    return x;
  }
};

#endif
