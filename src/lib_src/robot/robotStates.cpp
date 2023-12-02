#include "lib_header/robot_h/robotStates.h"

stateMachine states;

void stateHandler() {
    // default states
    states.setIntakeState(stateMachine::intake_state::OFF);
    states.setWingState(stateMachine::wing_state::WINGS_STOWED);
    states.setParkingBrakeState(stateMachine::parking_brake_state::BRAKE_OFF);

    // odom initialization
    resetOdomSensors();
	globalPose.setPoint(0.0, 0.0, 0);

    // set encoder data rates
    inertial.set_data_rate(5);
    frontEnc.set_data_rate(5);
    sideEnc.set_data_rate(5);
    cataEnc.set_data_rate(5);
    optical.set_integration_time(20);
    
    // init optical sensors
    optical.set_led_pwm(100);

    // local variables
    int loopDelay = 10;
    int intakeCount = 0;

    while(true) {
    int loopStartTime = pros::c::millis();

     // ******** Odometry ******** //
    // if(pros::competition::is_autonomous()) {
        updatePosition();
    // }

    // ******** Drive state handler ******** //
    // if(states.driveStateChanged()) {
    //     if(states.driveStateIs(stateMachine::drive_state::TWO_MOTOR)) {            
    //     }
    //     else if(states.driveStateIs(stateMachine::drive_state::SIX_MOTOR)) {            
    //     }
    //     states.oldDriveState = states.driveState;
    // }

    // ******** Intake state handler ******** //
    if(states.intakeStateChanged()) {
        if(states.intakeStateIs(stateMachine::intake_state::OFF)) {
            stopCata(pros::E_MOTOR_BRAKE_BRAKE);
            intakeCount = 0;
            states.oldIntakeState = states.intakeState;
        }
        else if(states.intakeStateIs(stateMachine::intake_state::INTAKING)) {
            setCata(127);
            intakeCount += 10;

            if(intakeCount > 200 && (leftCata.get_current_draw() + rightCata.get_current_draw() > 1500)) {
                states.setIntakeState(stateMachine::intake_state::OFF);
                controller.rumble("-");
            }
        }
        else if(states.intakeStateIs(stateMachine::intake_state::OUTTAKING)) {
            setCata(-127);
            states.oldIntakeState = states.intakeState;
        }
    }
    // Rumble for triball
    // if(states.intakeStateIs(stateMachine::intake_state::INTAKING)) {
    //     if(intakeCount > 200 && (leftCata.get_current_draw() + rightCata.get_current_draw() > 1500)) {
    //         states.setIntakeState(stateMachine::intake_state::OFF);
    //         controller.rumble("-");
    //     }
    // }
    // else {
    //     intakeCount = 0;
    // }

    // ******** Cata state handler ******** //
    if(states.cataStateChanged()) {
           if(states.cataStateIs(stateMachine::cata_state::FIRE)) {
                if(displayInfo) {pros::screen::print(TEXT_MEDIUM_CENTER, 4, "CATA FIRED");}
                setCata(-127);
                states.setCataState(stateMachine::cata_state::SHORT_PULLBACK);
            }

            if(states.cataStateIs(stateMachine::cata_state::SHORT_PULLBACK)) {
                if(displayInfo) {pros::screen::print(TEXT_MEDIUM_CENTER, 5, "SHORT PULLBACK, Volt: %d", cataMotors.get_voltages());}
                setCata(-127);
                if(cataEnc.get_position() > (SHORT_PULLBACK_TICKS - PULLBACK_THRESHOLD) || pullbackCount >= PULLBACK_TIMEOUT) {
                    stopCata(pros::E_MOTOR_BRAKE_COAST);
                    states.setCataState(stateMachine::cata_state::PULLED_BACK);
                }
                pullbackCount += loopDelay;
            }
            
            if(states.cataStateIs(stateMachine::cata_state::PULLED_BACK)) {
                if(displayInfo) {pros::screen::print(TEXT_MEDIUM_CENTER, 4, "PULLED BACK");}
                pullbackCount = 0;
                states.oldCataState = states.cataState;
            }
    }

        
    // ******** Wing state handler ******** //
    if(states.wingStateChanged()) {
        if(states.wingStateIs(stateMachine::wing_state::WINGS_STOWED)) {
            if(displayInfo) {pros::screen::print(TEXT_MEDIUM_CENTER, 7, "WINGS IN");}
            leftWing.set_value(false);
            rightWing.set_value(false);
        }
        else if(states.wingStateIs(stateMachine::wing_state::WINGS_OUT)) {
            if(displayInfo) {pros::screen::print(TEXT_MEDIUM_CENTER, 7, "WINGS OUT");}
            leftWing.set_value(true);
            rightWing.set_value(true);
        }
        else if(states.wingStateIs(stateMachine::wing_state::LEFT_OUT)) {
            if(displayInfo) {pros::screen::print(TEXT_MEDIUM_CENTER, 7, "LEFT WING OUT");}
            leftWing.set_value(true);
            rightWing.set_value(false);
        }
        else if(states.wingStateIs(stateMachine::wing_state::RIGHT_OUT)) {
            if(displayInfo) {pros::screen::print(TEXT_MEDIUM_CENTER, 7, "RIGHT WING OUT");}
            leftWing.set_value(false);
            rightWing.set_value(true);
        }
        states.oldWingState = states.wingState;
    }

    // ******** Parking brake state handler ******** //
    if(states.parkingBrakeStateChanged()) {
        if(states.parkingBrakeStateIs(stateMachine::parking_brake_state::BRAKE_OFF)) {
            if(displayInfo) {pros::screen::print(TEXT_MEDIUM_CENTER, 8, "BRAKES OFF");}
            setDrive(0, 0);
            controller.rumble(".");
        }
        else if(states.parkingBrakeStateIs(stateMachine::parking_brake_state::BRAKE_ON)) {
            if(displayInfo) {pros::screen::print(TEXT_MEDIUM_CENTER, 8, "BRAKES ON");}
            setDrive(20, 20);
            leftFrontDrive.move(-20);
            rightFrontDrive.move(-20);
            controller.rumble(".");
        }
        states.oldParkingBrakeState = states.parkingBrakeState;
    }

    // Drive check for engaging brakes
    if(std::fabs(leftFrontDrive.get_actual_velocity()) + std::fabs(rightFrontDrive.get_actual_velocity()) < DRIVE_BRAKE_THRESHOLD) {
        brakeReady = true;
    }
    else {
        brakeReady = false;
    }


    // ******** Matchload ******** //
    // while(matchloadState) {
    //     fireCount = 0;
    //     while(true) {
    //         // fire
    //         setCata(-127);

    //         // pull back
    //         while(cataEnc.get_position() < (FULL_PULLBACK_TICKS - 10000) && matchloadState) {pros::delay(5);}
    //         fireCount++;
    //         stopCata(pros::E_MOTOR_BRAKE_COAST);

    //         // optical sensor
    //         while(!(optical.get_proximity() < 150 && (optical.get_hue()) < 100 && optical.get_hue() > 80)){
    //             pros::delay(5);
    //         }

    //         if(!matchloadState) { // || fireCount >= fireTarget
    //             stopCata(pros::E_MOTOR_BRAKE_COAST);
    //             states.setCataState(stateMachine::cata_state::PULLED_BACK);
    //             // fireCount = fireTarget = 0;
    //             break;
    //         }
    //     }
    // }
    while(matchloadState) {
        while(true) {
            setCata(-127);
            pros::delay(5);

            if(!matchloadState) {
                stopCata(pros::E_MOTOR_BRAKE_COAST);
                break;
            }
        }  
    }

    // ******** DEBUG ******** //
    // if(displayInfo) {
    // // pros::screen::print(TEXT_MEDIUM_CENTER, 10, "Drive Velo: %d", (leftFrontDrive.get_actual_velocity() + rightFrontDrive.get_actual_velocity()) / 2);
    // // pros::screen::print(TEXT_MEDIUM_CENTER, 11, "Brake Ready?: %s", brakeReady ? "true" : "false");
    // pros::screen::print(TEXT_MEDIUM_CENTER, 5, "Puncher Enc: %d", cataEnc.get_position());
    // }
    pros::screen::print(TEXT_MEDIUM_CENTER, 5, "Cata Enc: %d", cataEnc.get_position());
    pros::screen::print(TEXT_MEDIUM_CENTER, 6, "Cata Current: %d", leftCata.get_current_draw() + rightCata.get_current_draw());
    
    // necessary task delay - do not change
    pros::delay(loopDelay); // while(pros::c::millis() < loopStartTime + loopDelay) {pros::delay(1);}
    }
}
