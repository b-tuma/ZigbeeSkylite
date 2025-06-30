import * as m from 'zigbee-herdsman-converters/lib/modernExtend';

export default {
    zigbeeModel: ['SkyLite'],
    model: 'SkyLite',
    vendor: 'BrunoTuma',
    description: 'Zigbified Blisslight Skylite 2.0',
    extend: [
        m.deviceEndpoints({ endpoints: { light: 10, laser: 11, motor: 12, effect: 13 } }),
        m.light({ powerOnBehavior: false, color: {modes:["xy"]}, endpointNames: ["light"], effect: false }),
        m.light({ powerOnBehavior: false, color: false, endpointNames: ["laser"], effect: false }),
        m.numeric({ endpointNames: ["motor"], name: "rotation", unit: "%", valueMin: 0, valueMax: 100, cluster: "genAnalogOutput", attribute: "presentValue" }),
        m.numeric({ endpointNames: ["effect"], name: "colorloop", unit: "%", valueMin: 0, valueMax: 100, cluster: "genAnalogOutput", attribute: "presentValue" })
    ],
    meta: { "multiEndpoint": true },
};