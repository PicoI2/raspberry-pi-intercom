include <variables.scad>;

speaker_pos_x = 0.19 * client_width;
speaker_pos_z = 0.15 * client_height;
micro_pos_x = 0.90 * client_width;
micro_pos_z = 0.15 * client_height;

// Front
color ("blue")
difference () {
    // Box
    cube([client_width, client_deph, client_height]);
    
    // Empty box
    translate ([thickness, thickness, thickness])
        cube ([client_width-2*thickness, client_deph, client_height-2*thickness]);
    
    // Screen hole
    translate ([0.1 * client_width, 0-margin, 0.3 * client_height])
        cube ([screen_width, thickness+2*margin, screen_height]);
    
    // Screw hole
    translate ([0.5 * client_width, 0.75 * client_deph, -margin])
        cylinder (h = 2*thickness, r = thickness);
    
    // Speaker grid
    translate ([speaker_pos_x - 1, -0.5*thickness, speaker_pos_z - 1])
        for (x = [-speaker_diameter/2 : 2 : speaker_diameter/2]) {
            for (z = [-speaker_diameter/2 : 2 : speaker_diameter/2]) {
                translate ([x, 0, z])
                cube ([1.5, 2*thickness, 1.5]);
            }
        };
    // Micro hole
    translate ([micro_pos_x, thickness + margin, micro_pos_z])
        rotate ([90,0,0])
        cylinder (h = thickness + 2*margin, r = (micro_diameter)/2);
};

// Support for speaker
color ("blue")
translate ([speaker_pos_x, 2*thickness, speaker_pos_z])
rotate ([90,0,0])
difference () {
    cylinder (h = thickness, r = (speaker_diameter+thickness)/2);
    translate ([0, 0, -margin]) cylinder (h = thickness+2*margin, r = (speaker_diameter)/2);
};

// Support for microphone
color ("blue")
translate ([micro_pos_x, 2*thickness, micro_pos_z])
rotate ([90,0,0])
difference () {
    cylinder (h = thickness, r = (micro_diameter+thickness)/2);
    translate ([0, 0, -margin]) cylinder (h = thickness+2*margin, r = (micro_diameter)/2);
}
