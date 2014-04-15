union() {
	difference() {
		difference() {	
			translate([0,0,9])
				cube([10, 11, 6], center=true);
			translate([-1,0,8])
				cube([74, 7, 5], center=true);
		}
		translate([-1,0,10])
			cube([74, 2, 5], center=true);	
	}
	difference() {
		difference() {
			cube([10,32,12], center=true);
			translate([2, 0, -2])
			   cube([70,24,12], center=true);
		}
		translate([2, 0, -2])
		   cube([70,28,2], center=true);	
	}
}