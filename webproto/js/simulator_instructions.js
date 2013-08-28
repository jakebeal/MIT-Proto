addInstruction(curOpcode++, "RED", function(machine) {
   machine.red = machine.stack.peek(0);
});

addInstruction(curOpcode++, "GREEN", function(machine) {
   machine.green = machine.stack.peek(0);
});

addInstruction(curOpcode++, "BLUE", function(machine) {
   machine.blue = machine.stack.peek(0);
});

addInstruction(curOpcode++, "NBR_RANGE", function(machine) {
    x = machine.current_neighbor.x;
    y = machine.current_neighbor.y;
    z = machine.current_neighbor.z;
    dist = Math.sqrt(x*x + y*y + z*z);
    machine.stack.push(dist);
});

addInstruction(curOpcode++, "NBR_LAG", function(machine) {
    machine.stack.push(machine.current_neighbor.dataAge);
});

addInstruction(curOpcode++, "HOOD_RADIUS", function(machine) {
    machine.stack.push(machine.radius);
});

addInstruction(curOpcode++, "MOV", function(machine) {
    var tuple = machine.stack.peek(0);
    if (tuple.length > 0) machine.dx = tuple[0]; else machine.dx = 0;
    if (tuple.length > 1) machine.dy = tuple[1]; else machine.dy = 0;
    if (tuple.length > 2) machine.dz = tuple[2]; else machine.dz = 0;
});
