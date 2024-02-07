#include "main.h"

/**
 * A callback function for LLEMU's center button.
 *
 * When this callback is fired, it will toggle line 2 of the LCD text between
 * "I was pressed!" and nothing.
 */
void on_center_button() {
	static bool pressed = false;
	pressed = !pressed;
	if(pressed) {
		pros::lcd::set_text(2, "I was pressed!");
	}
	else {
		pros::lcd::clear_line(2);
	}
}

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */
void initialize() {
	pros::Task inertialInit([] {inertial.reset(true);});
	pros::Task stateMachineTask(stateHandler);

	pros::Task GUI(initGUI);
	// setShooterBrakeMode(pros::E_MOTOR_BRAKE_COAST);
}

/**
 * Runs while the robot is in the disabled state of Field Management System or
 * the VEX Competition Switch, following either autonomous or opcontrol. When
 * the robot is enabled, this task will exit.
 */
void disabled() {
	climbDown(); // be careful with this one
}

/**
 * Runs after initialize(), and before autonomous when connected to the Field
 * Management System or the VEX Competition Switch. This is intended for
 * competition-specific initialization routines, such as an autonomous selector
 * on the LCD.
 *
 * This task will exit when the robot is enabled and autonomous or opcontrol
 * starts.
 */
void competition_initialize() {}

/**
 * Runs the user autonomous code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the autonomous
 * mode. Alternatively, this function may be called in initialize or opcontrol
 * for non-competition testing purposes.
 *
 * If the robot is disabled or communications is lost, the autonomous task
 * will be stopped. Re-enabling the robot will restart the task, not re-start it
 * from where it left off.
 */
pros::Task autoMovement(autoMovementTask);
void autonomous() {
	// **** Init **** //
	displayInfo = false;
	// odom init //
	resetOdomSensors();
	globalPose.setPoint(0.0, 0.0, 0);

	// **** Extend moveToPoint test **** //
	// setMoveToPoint(0, 24, 800, false);
	// waitUntilNear(4, 0);
	// extendMoveToPoint(12, 24, 100, 100, 1000, false);
	// waitUntilNear(4, 0);
	// extendMoveToPoint(0, 0, 100, 100, 1000, false);
	// waitUntilSettled(500);
	// setMove(0, 90, 1000);
	// waitUntilSettled(0);

	// globalPose.setPoint(103.5, 17, 1); // newSixBall() // top left of tile
	// states.setWingState(stateMachine::wing_state::RIGHT_OUT);

	/** Forward and backward test */
	// setMoveToPoint(0, 24, 40, 30, 5000, false);
	// waitUntilSettled(0);
	// setMoveToPoint(0, 0, 40, 30, 5000, true);
	// waitUntilSettled(0);
	
	// squigglesTest(); 		// FAILED, turns but doesn't move after chained movement
	// chainedMoveToPoint(); 	// FAILED, turns but doesn't move after first chained to move
	
	// ** waitUntilNear test (WORKS) ** //
	// setMoveToPoint(40, 24, 900, false);
	// waitUntilNear(5, 0);
	// setMoveToPoint(49, 5, 800, true);
	// waitUntilNear(5, 0);
	// setMoveToPoint(0, 0, 1200, true);
	// waitUntilSettled(50000);

	// globalPose.setPoint(120, 14, 321); // angled to face triball near bar; right front wheel in line with intersection of tiles; (24, 14)
	// sixBall(sixBall_mode::BAR);

	// **** Autoselector **** //
	if(autoToRun == 1) {
		newSixBall(sixBall_mode::BAR);
		// defenseAuto(defense_auto_mode::FOUR_BALL);
	}
	if(autoToRun == 2) {
		defenseAuto(defense_auto_mode::THREE_BALL);
	}
	if(autoToRun == 3) {
		defenseSafe();
	}
	if(autoToRun == 4) {
		progSkills();
	}
	if(autoToRun == 5) {
		newSixBall(sixBall_mode::MID);
	}
	if(autoToRun == 6) {
		newSixBall(sixBall_mode::BAR);
	}
}

/**
 * Runs the operator control code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the operator
 * control mode.
 *
 * If no competition control is connected, this function will run immediately
 * following initialize().
 *
 * If the robot is disabled or communications is lost, the
 * operator control task will be stopped. Re-enabling the robot will restart the
 * task, not resume it from where it left off.
 */
void opcontrol() {
	// **** init **** //
	autoMovement.suspend();
	controller.rumble("-");
	// displayInfo = true;

	// **** local variables **** //
	int opcontrolStartTime = pros::c::millis();
	int rumbleCount = 0;

	// **** local constants **** //
	int TEN_SECONDS_LEFT =  (105 - 10) * 1000; // 95,000 ms
	int THREE_SECONDS_LEFT = (105 - 3) * 1000; 	// 102,000 ms

	while(true) {
		// **** Auto Macro **** //
		if(controller.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_LEFT)) {
			autoMovement.resume();
			pros::Task progInDriver(progFirstHalf);
			while(controller.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_Y) < 50) {pros::delay(20);}
			progInDriver.suspend();
			autoMovement.suspend();
		}

		// **** Subsystems **** //
		splitArcade(pros::E_MOTOR_BRAKE_COAST); 										// Drive
		intakeOpControl(pros::E_CONTROLLER_DIGITAL_R2, pros::E_CONTROLLER_DIGITAL_R1);	// Intake
		shooterOpControl(pros::E_CONTROLLER_DIGITAL_B);									// Shooter
		wingOpControl(pros::E_CONTROLLER_DIGITAL_L1);	  								// Wings
		matchloadOpControl(pros::E_CONTROLLER_DIGITAL_L2);								// Matchload
		climbOpControl();							// Climb
		// brakeOpControl();						// Brake

		// **** Match timer **** //
		if(pros::c::millis() - opcontrolStartTime >= TEN_SECONDS_LEFT && pros::c::millis() - opcontrolStartTime < 110000) {
			// rumble once every second at last 10 seconds
			if(rumbleCount % 1000 == 0) {controller.rumble(".");}

			// rumble twice every second at last 3 seconds
			if(pros::c::millis() - opcontrolStartTime >= THREE_SECONDS_LEFT) {
				if(rumbleCount % 500 == 0) {controller.rumble(".");}
			}
			rumbleCount += 20;
		}

		// loop delay
		pros::delay(20);	
	}
}