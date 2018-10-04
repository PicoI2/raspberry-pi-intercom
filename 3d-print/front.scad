// Size are in millimeters
height = 135;   // z axis
width = 94;     // x axis
deph = 38;      // y axis
thickness = 3;
cam_diameter = 16;
button_diameter = 24.2;
speaker_diameter = 27;
micro_diameter = 12;
margin = 0.4;

// Front
color ("blue")
difference () {
    // Box
    cube([width, deph, height]);
    // Empty box
    translate ([thickness, thickness, thickness]) cube ([width-2*thickness, deph, height]);
    // Camera hole
    translate ([0.5 * width, 1.5 * thickness, 0.80 * height]) rotate ([90,0,0]) cylinder (h = 2*thickness, r = cam_diameter/2);
    // Button hole
    translate ([0.8 * width, 1.5 * thickness, 0.15 * height]) rotate ([90,0,0]) cylinder (h = 2*thickness, r = button_diameter/2);
    // Name hole
    // translate ([0.1 * width, 0, 0.1 * height]) cube ([0.5 * width, 1, 0.1 * height]);
    // Speaker grid
    translate ([0.5 * width, -0.5*thickness, 0.55 * height])
        for (x = [-speaker_diameter/2 : 2 : speaker_diameter/2]) {
            for (z = [-speaker_diameter/2 : 2 : speaker_diameter/2]) {
                translate ([x, 0, z]) cube ([1, 2*thickness, 1]);
            }
        };
    // Micro grid
    translate ([0.5 * width, -0.5*thickness, 0.3 * height])
        for (x = [-micro_diameter/2 : 2 : micro_diameter/2]) {
            for (z = [-micro_diameter/2 : 2 : micro_diameter/2]) {
                translate ([x, 0, z]) cube ([1, 2*thickness, 1]);
            }
        };    
};

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
