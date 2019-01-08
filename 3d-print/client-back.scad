include <variables.scad>;

// Back
difference () {
    translate ([-margin - thickness, - 0.5*client_deph, - thickness]) 
        cube ([client_width + 2*margin + 2*thickness, 1.5*client_deph + thickness, client_height + 2*margin + 2*thickness]);
    // Cut cube
    translate([-0.5*client_width, -client_deph-thickness ,-0.5*client_height])
        rotate([atan(client_deph/client_height),0,0])
        cube ([2*client_width, 2*client_deph, 2*client_height]);    
    // Remove place for face
    translate ([-margin, -client_deph, 0]) cube ([client_width+2*margin, 2*client_deph, client_height + 2*margin]);
    // Hole for wires
    translate ([0.4 * client_width, client_deph-0.5*thickness, thickness])
        cube ([0.2 * client_width, 2*thickness, 0.1 * client_height]);
    // Screw hole
    translate ([0.5 * client_width, 0.75 * client_deph, -thickness-margin])
        cylinder (h = 2*thickness, r = thickness);
};

