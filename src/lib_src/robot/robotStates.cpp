#include "lib_header/robot_h/robotStates.h"

stateMachine states;
bool runStateHandler = false;

void stateHandler() {
    // initialize old states
    states.oldIntakeState = stateMachine::intake_state::NULL_STATE;
    states.oldShooterState = stateMachine::shooter_state::NULL_STATE;
    states.oldWingState = stateMachine::wing_state::NULL_STATE;
    states.oldClimbState = stateMachine::climb_state::NULL_STATE;
    states.oldParkingBrakeState = stateMachine::parking_brake_state::NULL_STATE;

    // default states
    matchloadState = false;
    states.setIntakeState(stateMachine::intake_state::OFF);
    states.setShooterState(stateMachine::shooter_state::PULLED_BACK);
    states.setWingState(stateMachine::wing_state::WINGS_STOWED);
    states.setClimbState(stateMachine::climb_state::DOWN);
    states.setParkingBrakeState(stateMachine::parking_brake_state::BRAKE_OFF);

    // set sensor data rates
    inertial.set_data_rate(5);
    frontEnc.set_data_rate(5);
    sideEnc.set_data_rate(5);
    shooterEnc.set_data_rate(5);

    // default brake modes
    // intake.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    // leftIntake.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    // rightIntake.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    setIntakeBrakeMode(pros::E_MOTOR_BRAKE_COAST);
    setShooterBrakeMode(pros::E_MOTOR_BRAKE_COAST);

    // optical sensor initilization
    // optical.set_integration_time(40);   // 40 ms data refresh rate
    // optical.disable_gesture();
    // optical.set_led_pwm(100);           // brightness

    // odom initialization
    resetOdomSensors();
	globalPose.setPoint(0.0, 0.0, 0);

    // local variables
    int loopDelay = 10; // refresh delay for loop
    int fireCount = 0;  // time for 
    int intakeCount = 0;
    int matchloadDelay = 0;
    int rumbleCount = 0;
    int displayCount = 0;
    // bool passedZero = false;    // shooter has shot once?

    // matchload first loop
    bool matchloadFirstLoop = false;

    // signal stateHanlder is running
    controller.rumble("-");

    while(true) {
    // int loopStartTime = pros::c::millis();

     // ******** Odometry ******** //
    // if(pros::competition::is_autonomous()) {
        updatePosition();
    // }

    // ******** Intake state handler ******** //
    if(states.intakeStateChanged()) {
        if(states.intakeStateIs(stateMachine::intake_state::OFF)) {
            // intake.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
            // intake.brake();
            stopIntake(pros::E_MOTOR_BRAKE_COAST);
            intakeCount = 0;
        }
        else if(states.intakeStateIs(stateMachine::intake_state::INTAKING)) {
            // intake.move(127);
            setIntake(127);
        }
        else if(states.intakeStateIs(stateMachine::intake_state::OUTTAKING)) {
            // intake.move(-127);
            setIntake(-127);
        }
        states.oldIntakeState = states.intakeState;
    }

    // If triball in intake, rumble controller
    if(states.intakeStateIs(stateMachine::intake_state::INTAKING)) {
        intakeCount += loopDelay;
        // if(intakeCount > 200 && intake.get_current_draw() > 1500) {
        //     controller.rumble("-");
        if(intakeCount > 200) {
            if(oneIntakeMode) {
                if(intake.get_current_draw() > 1500) {controller.rumble("-");}
            }
            else {
                if(rightIntake.get_current_draw() > 1500) {controller.rumble("-");}
            }
        }
    }
    else {intakeCount = 0;}

    // TODO - Better firing logic (e.g. fire, wait until sensor jumps back to zero-ish, then pull back)
    // ******** Shooter state handler ******** // 
    if(states.shooterStateChanged() && oneIntakeMode) {
        if(states.shooterStateIs(stateMachine::shooter_state::FIRE)) {
            if(displayInfo) {pros::screen::print(TEXT_MEDIUM_CENTER, 3, "SHOOTER FIRED");}
            // if((shooterEnc.get_position()/100) > 330) {passedZero = true;}
            // setShooter(-127);
            // if(passedZero) {
            //     states.setShooterState(stateMachine::shooter_state::SHORT_PULLBACK);
            // }

            setShooter(-80);
            // fireCount += loopDelay;
            // if(fireCount > MIN_FIRE_TIME) {
            //     fireCount = 0;
            //     states.setShooterState(stateMachine::shooter_state::SHORT_PULLBACK);
            // }
            if(shooterEnc.get_position() < 10*100) {
                states.setShooterState(stateMachine::shooter_state::SHORT_PULLBACK);
            }
        }

        if(states.shooterStateIs(stateMachine::shooter_state::SHORT_PULLBACK)) {
            // passedZero = false;
            // if(displayInfo) {pros::screen::print(TEXT_MEDIUM_CENTER, 2, "SHORT PULLBACK, Volt: %d", cataMotors.get_voltages());}
            if(displayInfo) {pros::screen::print(TEXT_MEDIUM_CENTER, 3, "PULLING BACK");}
            setShooter(-127);
            if(shooterEnc.get_position() > (SHORT_PULLBACK_TICKS - PULLBACK_THRESHOLD) || pullbackCount >= PULLBACK_TIMEOUT) {
                stopShooter(pros::E_MOTOR_BRAKE_COAST);
                states.setShooterState(stateMachine::shooter_state::PULLED_BACK);
            }
            pullbackCount += loopDelay;
        }

        if(states.shooterStateIs(stateMachine::shooter_state::PULLED_BACK)) {
            if(displayInfo) {pros::screen::print(TEXT_MEDIUM_CENTER, 3, "PULLED BACK !!!!!!!");}
            // stopCata(pros::E_MOTOR_BRAKE_COAST);
            // passedZero = false;
            pullbackCount = 0;
            states.oldShooterState = states.shooterState;
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


    // ******** Climb state handler ******** //
    // if(states.climbStateChanged()) {
    //     if(states.climbStateIs(stateMachine::climb_state::DOWN)) {
    //         leftClimb.set_value(false);
    //         rightClimb.set_value(false);
    //         states.oldClimbState = states.climbState;
    //     }
    //     else if(states.climbStateIs(stateMachine::climb_state::UP)) {
    //         leftClimb.set_value(true);
    //         rightClimb.set_value(true);
    //         states.oldClimbState = states.climbState;
    //     }
    //     else if(states.climbStateIs(stateMachine::climb_state::AUTO_BALANCE)) {
    //         if(inertial.get_roll() > 5) {
    //             // matchloadState = true;
    //             pros::screen::set_eraser(COLOR_BLACK);
    //             pros::screen::erase();
    //             pros::screen::set_pen(COLOR_BLUE);
    //             pros::screen::fill_rect(20, 20, 400, 400);
    //         }
    //         else {
    //             pros::screen::set_eraser(COLOR_BLACK);
    //             pros::screen::erase();
    //             pros::screen::set_pen(COLOR_BLUE);
    //             pros::screen::fill_rect(20, 20, 400, 400);
    //             matchloadState = false;
    //             states.oldClimbState = states.climbState;
    //         }
    //     }
    // }


    // ******** Parking brake state handler ******** //
    // if(states.parkingBrakeStateChanged()) {
    //     if(states.parkingBrakeStateIs(stateMachine::parking_brake_state::BRAKE_OFF)) {
    //         if(displayInfo) {pros::screen::print(TEXT_MEDIUM_CENTER, 8, "BRAKES OFF");}
    //         setDrive(0, 0);
    //         controller.rumble(".");
    //     }
    //     else if(states.parkingBrakeStateIs(stateMachine::parking_brake_state::BRAKE_ON)) {
    //         if(displayInfo) {pros::screen::print(TEXT_MEDIUM_CENTER, 8, "BRAKES ON");}
    //         setDrive(20, 20);
    //         leftFrontDrive.move(-20);
    //         rightFrontDrive.move(-20);
    //         controller.rumble(".");
    //     }
    //     states.oldParkingBrakeState = states.parkingBrakeState;
    // }

    // Drive check for engaging brakes
    // if(std::fabs(leftFrontDrive.get_actual_velocity()) + std::fabs(rightFrontDrive.get_actual_velocity()) < DRIVE_BRAKE_THRESHOLD) {
    //     brakeReady = true;
    // }
    // else {brakeReady = false;}


    // ******** Matchload ******** //
    if(matchloadState && oneIntakeMode) {
        matchloadFirstLoop = true;
        // optical.set_led_pwm(100);
        // rumble every second to signal we are in matchload state
        // rumbleCount += loopDelay;

        // if(rumbleCount > 1000) {
        //     rumbleCount = 0;
        //     controller.rumble(".");
        // }

        // ** firing logic with optical sensor ** // 
        // if(states.shooterStateIs(stateMachine::shooter_state::PULLED_BACK)) {
        //     // closer to optical = higher proximity val; range from 0-255
        //     if(optical.get_proximity() > 250 ) { // && (optical.get_hue()) < 100 && optical.get_hue() > 80 
        //         // add delay
        //         pros::delay(100);
        //         states.setShooterState(stateMachine::shooter_state::FIRE);
        //         triballsFired++;
        //     }
        // }

        // ** regular firing logic ** //
        // if(states.shooterStateIs(stateMachine::shooter_state::PULLED_BACK)) {
        //     matchloadDelay += loopDelay;
        //     if(matchloadDelay >= FIRE_DELAY) {
        //         matchloadDelay = 0;
        //         states.setShooterState(stateMachine::shooter_state::FIRE);
        //     }
        // }
        setShooter(-70);
    }
    else {
        if(matchloadFirstLoop) {
            // optical.set_led_pwm(0);
            matchloadDelay = 0;
            triballsFired = 0;
            stopShooter(pros::E_MOTOR_BRAKE_COAST);
            matchloadFirstLoop = false;
        }
    }

    // ******** DEBUG ******** //
    if(displayInfo) {
    // // pros::screen::print(TEXT_MEDIUM_CENTER, 10, "Drive Velo: %d", (leftFrontDrive.get_actual_velocity() + rightFrontDrive.get_actual_velocity()) / 2);
    // // pros::screen::print(TEXT_MEDIUM_CENTER, 11, "Brake Ready?: %s", brakeReady ? "true" : "false");
    // pros::screen::print(TEXT_MEDIUM_CENTER, 5, "Puncher Enc: %d", cataEnc.get_position());

        // new stuff
        pros::screen::print(TEXT_MEDIUM_CENTER, 4, "Shooter Enc: %d", shooterEnc.get_position());
        pros::screen::print(TEXT_MEDIUM_CENTER, 5, "Shooter Angle: %d", shooterEnc.get_angle());
        pros::screen::print(TEXT_MEDIUM_CENTER, 6, "Shooter Current: %d", leftShooter.get_current_draw() + rightShooter.get_current_draw());
        // pros::screen::print(TEXT_MEDIUM_CENTER, 7, "Opical prox: %d", optical.get_proximity());
    }
    pros::screen::erase_line(0, 3, 800, 3);
    pros::screen::print(TEXT_MEDIUM, 3, "Shooter Enc: %5d, Ang: %5d | Passed ?  ", shooterEnc.get_position(), shooterEnc.get_angle());
    // pros::screen::erase_line(0, 4, 600, 5);
    // pros::screen::print(TEXT_MEDIUM, 4, "Front Enc rotation: %d", frontEnc.get_position());
    // pros::screen::print(TEXT_MEDIUM, 5, "Opical prox: %d", optical.get_proximity());

    // Refresh display every 100 ms
    // displayCount += loopDelay;
    // if(displayCount > 100) {
    //     displayCount = 0;
    //     pros::screen::set_eraser(COLOR_BLACK);
    //     pros::screen::erase();
    // }

    // necessary task delay - do not change

    pros::delay(loopDelay); // while(pros::c::millis() < loopStartTime + loopDelay) {pros::delay(1);}    
    }
}
