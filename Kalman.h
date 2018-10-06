#ifndef _Kalman_h
#define _Kalman_h

/*
 * Check out
 * 
 * 
 */
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
  
  // http://credentiality2.blogspot.com/2010/08/simple-kalman-filter-example.html
//  float predict( float f ) {
//    float g  = pn / ( pn + r ); // Weight
//    x = g * ( f - x ) + x;      // Estimate
//    p = pn * r / ( pn + r );
//    return x;
//  }

//  float predict( float f ) {
//    float pc = p + pn;
//    float g  = ( pc == 0 ? 1 : pc / ( pc + r ) ); // Weight
//    x = g * ( f - x ) + x;    // Estimate
//    p = ( 1 - g ) * pc;       
//    return x;
//  }

  // https://magesblog.com/post/2014-12-02-measuring-temperature-with-my-arduino/
  float predict( float f ) {
    p = p + pn;
    float K = p / (p + r);    // Weight
    x = K * f + (1 - K) * x;  // Estimate
    p = (1 - K) * p;
    return x;
  }
};

#endif
