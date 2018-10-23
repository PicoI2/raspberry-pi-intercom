include <variables.scad>;

// Back
difference () {
    translate ([-margin - thickness, - 0.5*deph, - thickness]) 
        cube ([width + 2*margin + 2*thickness, 1.5*deph + thickness, height + 2*margin + 2*thickness]);
    // Cut cube
    translate([-0.5*width, -deph-thickness ,-0.5*height])
        rotate([atan(deph/height),0,0])
        cube ([2*width, 2*deph, 2*height]);    
    // Remove place for face
    translate ([-margin, -deph, 0]) cube ([width+2*margin, 2*deph, height + 2*margin]);
    // Hole for wires
    translate ([thickness, deph-0.5*thickness, 0.4 * height]) cube ([20, 2*thickness, 30]);
    // Screw hole
    translate ([0.5 * width, 0.75 * deph, -thickness-margin]) cylinder (h = 2*thickness, r = thickness);
};

// Support for rpi
rpi_sup_w = 49;
rpi_sup_h = 58;
rpi_sup_d = 2.1;
rpi_sup_l = 7;

module support () { rotate ([90,0,0]) cylinder (h = rpi_sup_l, r = rpi_sup_d / 2); };
translate ([width - rpi_sup_w - thickness - 5, deph + margin, height - rpi_sup_h - thickness - 7]) {
    translate ([rpi_sup_w,0,0]) support();
    translate ([0,0,0]) support();
    translate ([rpi_sup_w,0,rpi_sup_h]) support();
    translate ([0,0,rpi_sup_h]) support();
};
