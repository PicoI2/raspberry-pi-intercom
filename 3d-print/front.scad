// Size are in millimeters
height = 135;   // z axis
width = 94;     // x axis
deph = 38;      // y axis
thickness = 3;
cam_diameter = 16;
button_diameter = 24.2;
speaker_diameter = 27;
micro_diameter = 12;

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
    translate ([0.1 * width, 0, 0.1 * height]) cube ([0.5 * width, 1, 0.1 * height]);
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
    
}
