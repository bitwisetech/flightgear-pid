// pidcontroller.cxx - implementation of PID controller
//
// Written by Torsten Dreyer
// Based heavily on work created by Curtis Olson, started January 2004.
//
// Copyright (C) 2004  Curtis L. Olson  - http://www.flightgear.org/~curt
// Copyright (C) 2010  Torsten Dreyer - Torsten (at) t3r (dot) de
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include "pidcontroller.hxx"
#include <Main/fg_props.hxx>

using namespace FGXMLAutopilot;

using std::endl;
using std::cout;

PIDController::PIDController():
    AnalogComponent(),
    alpha( 0.1 ),
    beta( 1.0 ),
    gamma( 0.0 ),
    ep_n_1( 0.0 ),
    edf_n_1( 0.0 ),
    edf_n_2( 0.0 ),
    u_n_1( 0.0 ),
    desiredTs( 0.0 ),
    elapsedTime( 0.0 ),
    u_n( 0),
    r_n( 0),
    y_n( 0),
    //
    delta_P( 0),
    delta_I( 0),
    delta_D( 0),
    accum_P( 0),
    accum_I( 0),
    accum_D( 0)
{
}

/*
 * Roy Vegard Ovesen:
 *
 * Ok! Here is the PID controller algorithm that I would like to see
 * implemented:
 *
 *   delta_u_n = Kp * [ (ep_n - ep_n-1) + ((Ts/Ti)*e_n)
 *               + (Td/Ts)*(edf_n - 2*edf_n-1 + edf_n-2) ]
 *
 *   u_n = u_n-1 + delta_u_n
 *
 * where:
 *
 * delta_u : The incremental output
 * Kp      : Proportional gain
 * ep      : Proportional error with reference weighing
 *           ep = beta * r - y
 *           where:
 *           beta : Weighing factor
 *           r    : Reference (setpoint)
 *           y    : Process value, measured
 * e       : Error
 *           e = r - y
 * Ts      : Sampling interval
 * Ti      : Integrator time
 * Td      : Derivator time
 * edf     : Derivate error with reference weighing and filtering
 *           edf_n = edf_n-1 / ((Ts/Tf) + 1) + ed_n * (Ts/Tf) / ((Ts/Tf) + 1)
 *           where:
 *           Tf : Filter time
 *           Tf = alpha * Td , where alpha usually is set to 0.1
 *           ed : Unfiltered derivate error with reference weighing
 *             ed = gamma * r - y
 *             where:
 *             gamma : Weighing factor
 * 
 * u       : absolute output
 * 
 * Index n means the n'th value.
 * 
 * 
 * Inputs:
 * enabled ,
 * y_n , r_n , beta=1 , gamma=0 , alpha=0.1 ,
 * Kp , Ti , Td , Ts (is the sampling time available?)
 * u_min , u_max
 * 
 * Output:
 * u_n
 */

void PIDController::update( bool firstTime, double dt ) 
{
    if( firstTime ) {
      ep_n_1 = 0.0;
      edf_n_2 = edf_n_1 = 0.0;

      // first time being enabled, seed with current property tree value
      u_n = u_n_1 = get_output_value();
      if ( _pid_shadows ) {
        // shadowing: preset Rn to zero error 
        r_n = y_n;
        if( _debug ) cout << "Ntry:" << subsystemId() << " " 
  	    	  << " Rn:" << std::fixed << std::showpos << std::setprecision( 3 ) << r_n
	 			    << " Yn:" << std::fixed << std::showpos << std::setprecision( 3 ) << y_n
            << " U1:" << std::fixed << std::showpos << std::setprecision( 3 ) << u_n_1
            << endl; 
      }
      accum_P = 0.0;
      accum_I = 0.0;
      accum_D = 0.0;

    //  try to prevent PID getting two bites at entry       } else {
    }

    double u_min = _minInput.get_value();
    double u_max = _maxInput.get_value();

    elapsedTime += dt;
    if( elapsedTime <= desiredTs ) {
        // do nothing if time step is not positive (i.e. no time has
        // elapsed)
        return;
    }
    double Ts = elapsedTime; // sampling interval (sec)
    elapsedTime = 0.0;

    if( Ts > SGLimitsd::min()) {
        //  if( _debug ) cout <<  "Updating " << subsystemId()
        //      << " Ts " << Ts << endl;

         y_n = _valueInput.get_value();
         r_n = _referenceInput.get_value();
        double kp = Kp.get_value();

        //   if ( _debug ) cout << "  input = " << y_n << " ref = " << r_n << endl;

        // Calculates Reference(SV) weighted error and incremental P value :
        double ep_n = beta * r_n - y_n;
        //  if ( _debug ) cout << "  ep_n = " << ep_n;
        //  if ( _debug ) cout << "  ep_n_1 = " << ep_n_1;
        //
        delta_P = kp * (ep_n - ep_n_1);
        
        double e_n = r_n - y_n;
        double ti = Ti.get_value();
        // ti <= 0 means no I-Term else calculate unweighted error and incremmental I value       
        if ( ti > 0.0 ) {
          // Calculates error:
          //  if ( _debug ) cout << " e_n = " << e_n;
          delta_I = kp * ((Ts/ti) * e_n);
        } else {
          delta_I = 0.0;
        }  

        double edf_n = 0.0;
        double td = Td.get_value();
        if ( td > 0.0 ) { // do we need to calcluate derivative error?

          // Calculates derivate error:
            double ed_n = gamma * r_n - y_n;
            //  if ( _debug ) cout << " ed_n = " << ed_n;

            // Calculates filter time:
            double Tf = alpha * td;
            // if ( _debug ) cout << " Tf = " << Tf;

            // Filters the derivate error:
            edf_n = edf_n_1 / (Ts/Tf + 1)
                + ed_n * (Ts/Tf) / (Ts/Tf + 1);
            // if ( _debug ) cout << " edf_n = " << edf_n;
            delta_D = kp * ((td/Ts) * (edf_n - 2*edf_n_1 + edf_n_2)); 
        } else {
            delta_D = edf_n_2 = edf_n_1 = edf_n = 0.0;
        }
        
        // Calculates the incremental output:
        double delta_u_n = delta_P + delta_I + delta_D;
        //if ( ti > 0.0 ) {
        //    delta_u_n = Kp.get_value() * ( (ep_n - ep_n_1)
        //                       + ((Ts/ti) * e_n)
        //                       + ((td/Ts) * (edf_n - 2*edf_n_1 + edf_n_2)) );
        accum_P += delta_P;
        accum_I += delta_I;
        accum_D += delta_D;
        delta_u_n = delta_P + delta_I +delta_D;
        if (_show_terms) {
            // copy accumulated components of Process Output to props tree 
            fgSetFloat("systems/tune/pid/P-Term", accum_P);
            fgSetFloat("systems/tune/pid/I-Term", accum_I);
            fgSetFloat("systems/tune/pid/D-Term", accum_D);
				}		
                               

          //  if ( _debug ) {
          //      cout << " delta_u_n = " << delta_u_n << endl;
          //      cout << "P:" << Kp.get_value() * (ep_n - ep_n_1)
          //           << " I:" << Kp.get_value() * ((Ts/ti) * e_n)
          //           << " D:" << Kp.get_value() * ((td/Ts) * (edf_n - 2*edf_n_1 + edf_n_2))
          //           << endl;
          //             }

        // Integrator anti-windup logic:
        if ( delta_u_n > (u_max - u_n_1) ) {
            delta_u_n = u_max - u_n_1;
            //  if ( _debug ) cout << " max saturation " << endl;
        } else if ( delta_u_n < (u_min - u_n_1) ) {
            delta_u_n = u_min - u_n_1;
            //  if ( _debug ) cout << " min saturation " << endl;
        }

        // Calculates absolute output:
        u_n = u_n_1 + delta_u_n;
        //  if ( _debug ) cout << "  output = " << u_n << endl;

        // Updates indexed values;
        u_n_1   = u_n;
        ep_n_1  = ep_n;
        edf_n_2 = edf_n_1;
        edf_n_1 = edf_n;

        //  condenseed debug output 
        if ( _debug ) {
          cout << "PID: "<< subsystemId() << " "
            // << " Ts:"  << std::fixed << std::showpos << std::setprecision( 3 ) << Ts 
					  << " Rn:"  << std::fixed << std::showpos << std::setprecision( 3 ) << r_n
				    << " Yn:"  << std::fixed << std::showpos << std::setprecision( 3 ) << y_n
            << " En:"  << std::fixed << std::showpos << std::setprecision( 3 ) << e_n
            << " Pt:"   << std::fixed << std::showpos << std::setprecision( 3 ) 
              << accum_P
            << " It:"   << std::fixed << std::showpos << std::setprecision( 3 ) 
              << accum_I
            << " Dt:"   << std::fixed << std::showpos << std::setprecision( 3 ) 
              << accum_D
            << " Un:" << std::fixed << std::showpos << std::setprecision( 3 ) << u_n
            << endl;
				}	
  // end first-time-skip // }  
  }    
    set_output_value( u_n );
}

//------------------------------------------------------------------------------
bool PIDController::configure( SGPropertyNode& cfg_node,
                               const std::string& cfg_name,
                               SGPropertyNode& prop_root )
{
  if( cfg_name == "config" ) {
    Component::configure(prop_root, cfg_node);
    return true;
  }

  if (cfg_name == "Ts") {
    desiredTs = cfg_node.getDoubleValue();
    return true;
  } 

  if (cfg_name == "Kp") {
    Kp.push_back( new InputValue(prop_root, cfg_node) );
    return true;
  } 

  if (cfg_name == "Ti") {
    Ti.push_back( new InputValue(prop_root, cfg_node) );
    return true;
  } 

  if (cfg_name == "Td") {
    Td.push_back( new InputValue(prop_root, cfg_node) );
    return true;
  } 

  if (cfg_name == "beta") {
    beta = cfg_node.getDoubleValue();
    return true;
  } 

  if (cfg_name == "alpha") {
    alpha = cfg_node.getDoubleValue();
    return true;
  } 

  if (cfg_name == "gamma") {
    gamma = cfg_node.getDoubleValue();
    return true;
  }

  return AnalogComponent::configure(cfg_node, cfg_name, prop_root);
}

// HP: Override function so that pid may shadow output prop when disabled 
void PIDController::disabled( double dt) {
  if ( _pid_shadows ) {
    // Reference signal forced to Input value so Error signal will be zero while disabled
    y_n = _valueInput.get_value();
    r_n = y_n;
    // Output value matches output property which will increment once enabled
    u_n_1 = get_output_value();
    if( _debug ) cout << "Shdw:" << subsystemId() << " " 
					  << " Rn:"  << std::fixed << std::showpos << std::setprecision( 3 ) << r_n
				    << " Yn:"  << std::fixed << std::showpos << std::setprecision( 3 ) << y_n
            << " U1:" << std::fixed << std::showpos << std::setprecision( 3 ) << u_n_1
            << endl; 
  }   
} 

// Register the subsystem.
SGSubsystemMgr::Registrant<PIDController> registrantPIDController;
