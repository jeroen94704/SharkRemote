union() {
	difference() {
		difference() {	
			translate([0,0,9])
				cube([35, 11, 6], center=true);
			translate([-1,0,8])
				cube([74, 7, 5], center=true);
		}
		translate([-1,0,10])
			cube([74, 2, 5], center=true);	
	}
	difference() {
		difference() {
			cube([35,23,12], center=true);
			translate([2, 0, -2])
			   cube([70,16,12], center=true);
		}
		translate([2, 0, 0])
		   cube([70,19,8], center=true);	
	}
}