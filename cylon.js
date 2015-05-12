
console = require('better-console');
var Cylon = require('cylon');
var sys = require("sys");

console.log(1);

Cylon.robot({
  connections: {
    drone: { adaptor: 'rolling-spider' }
  },

  devices: {
    drone: { driver: 'rolling-spider' }
  },

  work: function(my) {

    var stdin = process.openStdin();

    stdin.addListener("data", function(d) {
        // note:  d is an object, and when converted to a string it will
        // end with a linefeed.  so we (rather crudely) account for that
        // with toString() and then substring()
        // console.log(d.toString());
        // args = d.split(" ");
        //
        // switch(args[0]) {
        //   case "5":
        //       my.drone.trim();
        //       my.drone.takeOff();
        //       break;
        //
        //   case "8":
        //       my.drone.trim();
        //       my.drone.takeOff();
        //       break;
        //   case "2":
        //       my.drone.trim();
        //       my.drone.takeOff();
        //       break;
        //
        //   case "4":
        //       my.drone.trim();
        //       my.drone.takeOff();
        //       break;
        //   case "6":
        //       my.drone.trim();
        //       my.drone.takeOff();
        //       break;
        //
        //   case "0":
        //       my.drone.trim();
        //       my.drone.takeOff();
        //       break;
        // }
        eval(d.toString());
      });



    // console.log(my);
    // console.log(my.drone);
    // my.drone.trim();
    //
    // my.drone.takeOff();
    //
    // after(1000, function () {
    //
    //     my.drone.clockwise(100);
    //
    //     after(2500, function () {
    //
    //         my.drone.clockwise(0);
    //
    //         my.drone.land();
    //
    //         after(1500, function () {
    //
    //             Cylon.halt();
    //
    //         });
    //
    //     });
    //
    // });

  }
}).start();
