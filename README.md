# DroneByPitch

Drone By Pitch is a protocol to control drones with musical notes. The proof-of-concept implementation is being developped with the Parrot Rolling Spider in mind, but the idea can work with any remote controlled vehicle.

## The protocol

The drone starts when it hears 3 identical short notes. For the sake of easy examples, we will consider that this note is a C, but you can chose whatever. This note will be the reference note, and all instructions will be described in relation to this reference.

* Reference note (C) : Move forward
* Lower fourth (G) : Move backward
* Lower second (B) : Move left
* Upper third (E) : Move right
