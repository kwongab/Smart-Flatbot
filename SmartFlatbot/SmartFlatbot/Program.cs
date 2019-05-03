using System;
using System.Text;
using Scarlet.Utilities;
using Scarlet.IO.BeagleBone;
using Scarlet.IO;
using System.Net.Sockets;
using System.Net;
using Scarlet.Components.Motors;
using System.IO;
using Scarlet.Controllers;

namespace SmartFlatbot
{
    class Program
    {

        public static void Main(string[] args)
        {
            Boolean turnMode = false;
            Boolean lineMode = false;

            double SetTurn = 0;
            double PTurn = 0;
            double ITurn = 0;
            double DTurn = 0;

            double SetDis = 0;
            double PDis = 0;
            double IDis = 0;
            double DDis = 0;

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

            // Make rover (hopefully) stay still in begining.
            Motor[0].SetSpeed(0);
            Motor[1].SetSpeed(0);

            // Set the rover in turn only mode for testing
            if (args.Length > 0)
            {
                if (args[0].Equals("turn"))
                {
                    turnMode = true;
                }
                else if (args[0].Equals("line"))
                {
                    lineMode = true;
                }
            }


            /* Get the PID values from file PIDValues.txt
             * All values will be in double format
             * Data must be in this format below:
             * ------------------------------------------
             * Line 1 Ignored
             * Line 2 Ignored
             * Line 3 Direction target value
             * Line 4 Direction P value
             * Line 5 Direction I value
             * Line 6 Direction D value
             * Line 7 Ignored
             * Line 8 Distance target value
             * Line 9 Distance P value
             * Line 10 Distance I value
             * Line 11 Distance D value
             * All lines after are ignored
             * -----------------------------------------
             */
            try
            {   // Open the text file using a stream reader.
                using (StreamReader sr = new StreamReader("PIDValues.txt"))
                {
                    // Read the stream to a string, and write the string to the console.
                    sr.ReadLine();
                    sr.ReadLine(); // Ignore first two lines
                    SetTurn = Convert.ToDouble(sr.ReadLine());
                    PTurn = Convert.ToDouble(sr.ReadLine());
                    ITurn = Convert.ToDouble(sr.ReadLine());
                    DTurn = Convert.ToDouble(sr.ReadLine());
                    sr.ReadLine();
                    SetDis = Convert.ToDouble(sr.ReadLine());
                    PDis = Convert.ToDouble(sr.ReadLine());
                    IDis = Convert.ToDouble(sr.ReadLine());
                    DDis = Convert.ToDouble(sr.ReadLine());
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("PIDValues.txt is either missing or in improper format:");
                Console.WriteLine(e.Message);
            }

            Console.WriteLine("Direction Target, P, I, D: " + SetTurn + " " + PTurn + " " + ITurn + " " + DTurn );
            Console.WriteLine("Distance Target, P, I, D: " + SetDis + " " + PDis + " " + IDis + " " + DDis);

            // Set the PIDs
            PID directionPID = new PID(PTurn, ITurn, DTurn);
            directionPID.SetTarget(SetTurn);

            PID distancePID = new PID(PDis, IDis, DDis);
            directionPID.SetTarget(SetDis);


            // Start UDP communication
            // Listen for any IP with port 9000
            UdpClient udpServer = new UdpClient(9000);
            IPEndPoint remoteEP = new IPEndPoint(IPAddress.Any, 9000);

            // Main Loop. Keep running until program is stopped
            do
            {
                // Get data from UDP. Convert it to a String
                // Note this is a blocking call. 
                var data = udpServer.Receive(ref remoteEP);
                String dataString = Encoding.ASCII.GetString(data);

                Console.WriteLine(dataString);

                // Set the wheel values to be zero for now
                Double leftWheel = 0;
                Double rightWheel = 0;

                // Make sure the data received is not an empty string
                if (!String.IsNullOrEmpty(dataString))
                {

                    // Split the String into distance and direction
                    // Is in format "Distance,Direction"
                    // Distance is from 1 to 150 ish
                    // Direction is from -10 to 10 degrees
                    String[] speedDis = dataString.Split(',');
                    Double distance = Convert.ToDouble(speedDis[0]);
                    Double direction = (Convert.ToDouble(speedDis[1]));

                    // If mode is set to not turn only the run
                    if (!turnMode)
                    {
                        distancePID.Feed(distance);
                        if (!Double.IsNaN(distancePID.Output))
                        {
                            rightWheel += distancePID.Output;
                            leftWheel += distancePID.Output;
                        }

                    }

                    if (!lineMode)
                    {
                        directionPID.Feed(direction);
                        if (!Double.IsNaN(directionPID.Output))
                        {
                            rightWheel += directionPID.Output;
                            leftWheel -= directionPID.Output;
                        }
                    }

                }
                // If data received is not a full string, stop motors.
                else
                {
                    leftWheel = 0;
                    rightWheel = 0;
                }


                // Set the speed for the motor controllers
                // Max speed is one
                Motor[0].SetSpeed((float)(rightWheel));
                Motor[1].SetSpeed((float)(leftWheel));

            } while (true);
        }
    }
}
