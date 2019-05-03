using Scarlet.Components.Motors;
using Scarlet.IO;
using Scarlet.IO.BeagleBone;
using Scarlet.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace StopFlatbot
{
    class Program
    {
        static void Main(string[] args)
        {
            // Begin setup of Beaglebone pins
            StateStore.Start("SmartFlatbot");
            BeagleBone.Initialize(SystemMode.DEFAULT, true);

            // Add Beaglebone mapings with Scarlet
            BBBPinManager.AddMappingPWM(BBBPin.P9_14);
            BBBPinManager.AddMappingPWM(BBBPin.P9_16);
            BBBPinManager.AddMappingGPIO(BBBPin.P9_15, true, ResistorState.NONE, true);
            BBBPinManager.AddMappingGPIO(BBBPin.P9_27, true, ResistorState.NONE, true);

            // Apply mappings to Beaglebone
            BBBPinManager.ApplyPinSettings(BBBPinManager.ApplicationMode.APPLY_IF_NONE);

            IDigitalOut Motor1Output = new DigitalOutBBB(BBBPin.P9_15);
            IDigitalOut Motor2Output = new DigitalOutBBB(BBBPin.P9_27);

            IPWMOutput OutA = PWMBBB.PWMDevice1.OutputA;
            IPWMOutput OutB = PWMBBB.PWMDevice1.OutputB;

            Motor1Output.SetOutput(false);
            Motor2Output.SetOutput(false);

            PWMBBB.PWMDevice1.SetFrequency(10000); ;
            OutA.SetEnabled(true);
            OutB.SetEnabled(true);

            // Setup Motor Controls 
            CytronMD30C[] Motor = new CytronMD30C[2];
            Motor[0] = new CytronMD30C(OutA, Motor1Output, (float).5);
            Motor[1] = new CytronMD30C(OutB, Motor2Output, (float).5);

            Motor[0].SetSpeed(0);
            Motor[1].SetSpeed(0);

        }
    }
}
