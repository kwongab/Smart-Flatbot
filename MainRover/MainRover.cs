using System;
using Scarlet;
using Scarlet.IO.BeagleBone;
using System.Collections.Generic;
using Scarlet.Components;
using Scarlet.Components.Sensors;
using Scarlet.Communications;
using Scarlet.Utilities;
using System.Threading;
using System.Net.Sockets;
using System.Net;
using System.Text;
using Scarlet.Controllers;
using System.IO;

namespace MainRover
{
    public class MainRover
    {
        public static string SERVER_IP = "192.168.0.5";
        public const int NUM_PACKETS_TO_PROCESS = 20;
        public static Boolean turnMode = false;
        public static Boolean lineMode = false;

        public static bool Quit;
        public static List<ISensor> Sensors;
        public static QueueBuffer StopPackets;
        public static QueueBuffer ModePackets;
        public static QueueBuffer DrivePackets;
        public static QueueBuffer PathPackets;

        public enum DriveMode {BaseDrive, toGPS, findTennisBall, toTennisBall, destination};
        public static DriveMode CurDriveMode;
        public static Tuple<float, float> previousCoords;
        public static float PathSpeed, PathAngle;

        public static void PinConfig()
        {
            BBBPinManager.AddBusCAN(0);
            //BBBPinManager.AddMappingUART(Pins.MTK3339_RX);
            //BBBPinManager.AddMappingUART(Pins.MTK3339_TX);
            //BBBPinManager.AddMappingsI2C(Pins.BNO055_SCL, Pins.BNO055_SDA);
            //BBBPinManager.AddMappingGPIO(Pins.SteeringLimitSwitch, false, Scarlet.IO.ResistorState.PULL_UP, true);
            //BBBPinManager.AddMappingPWM(Pins.SteeringMotor);
            //BBBPinManager.ApplyPinSettings(BBBPinManager.ApplicationMode.NO_CHANGES);
            BBBPinManager.ApplyPinSettings(BBBPinManager.ApplicationMode.APPLY_IF_NONE);
        }

        public static void InitBeagleBone()
        {
            StateStore.Start("MainRover");
            BeagleBone.Initialize(SystemMode.NO_HDMI, true);
            PinConfig();
            Sensors = new List<ISensor>();
            try
            {
                //Sensors.Add(new BNO055(I2CBBB.I2CBus2));
                //Sensors.Add(new MTK3339(UARTBBB.UARTBus4));
            }
            catch (Exception e)
            {
                Log.Output(Log.Severity.ERROR, Log.Source.SENSORS, "Failed to initalize sensors (gps and/or mag)");
                Log.Exception(Log.Source.SENSORS, e);
            }
            /*
            // Add back in when limit switch is complete
            LimitSwitch Switch = new LimitSwitch(new DigitalInBBB(Pins.SteeringLimitSwitch));
            Switch.SwitchToggle += (object sender, LimitSwitchToggle e) => Console.WriteLine("PRESSED!");
            Sensors.Add(Switch);
            */
            foreach (ISensor Sensor in Sensors)
            {
                if (Sensor is MTK3339)
                {
                    previousCoords = ((MTK3339)Sensor).GetCoords();
                }
            }
        }

        public static void SetupClient()
        {
            Client.Start(SERVER_IP, 1025, 1026, "MainRover");
            DrivePackets = new QueueBuffer();
            StopPackets = new QueueBuffer();
            ModePackets = new QueueBuffer();
            PathPackets = new QueueBuffer();
            Parse.SetParseHandler(0x80, (Packet) => StopPackets.Enqueue(Packet, 0));
            Parse.SetParseHandler(0x99, (Packet) => ModePackets.Enqueue(Packet, 0));
            for (byte i = 0x8E; i <= 0x94; i++)
                Parse.SetParseHandler(i, (Packet) => DrivePackets.Enqueue(Packet, 0));
            for (byte i = 0x9A; i <= 0x9D; i++)
                Parse.SetParseHandler(i, (Packet) => DrivePackets.Enqueue(Packet, 0));
            for (byte i = 0x95; i <= 0x97; i++)
                Parse.SetParseHandler(i, (Packet) => PathPackets.Enqueue(Packet, 0));
            PathSpeed = 0;
            PathAngle = 0;
        }

        public static void ProcessInstructions()
        {
            if (!StopPackets.IsEmpty())
            {
                StopPackets = new QueueBuffer();
                // Stop the Rover

            }
            else if (!ModePackets.IsEmpty())
            {
                ProcessModePackets();
            }
            else
            {
                switch (CurDriveMode)
                {   // TODO For each case statement, clear undeeded queue buffers
                    case DriveMode.BaseDrive:
                        ProcessBasePackets();
                        PathPackets = new QueueBuffer();
                        break;
                    case DriveMode.toGPS:
                        ProcessPathPackets();
                        DrivePackets = new QueueBuffer();
                        break;
                    case DriveMode.findTennisBall:
                        DrivePackets = new QueueBuffer();
                        PathPackets = new QueueBuffer();
                        break;
                    case DriveMode.toTennisBall:
                        DrivePackets = new QueueBuffer();
                        PathPackets = new QueueBuffer();
                        break;
                    case DriveMode.destination:
                        DrivePackets = new QueueBuffer();
                        PathPackets = new QueueBuffer();
                        break;
                }
            }
        }

        public static void ProcessModePackets()
        {
            for (int i = 0; !ModePackets.IsEmpty() && i < NUM_PACKETS_TO_PROCESS; i++)
            {
                Packet p = ModePackets.Dequeue();                
                CurDriveMode = (DriveMode)p.Data.Payload[0];
                //temporary fix to test, actually fix it later to get corect values form payload
                if(p.Data.Payload[0] > 0) { CurDriveMode = DriveMode.toGPS; }         
                
            }
        }

        public static void ProcessBasePackets()
        {
            for (int i = 0; !DrivePackets.IsEmpty() && i < NUM_PACKETS_TO_PROCESS; i++)
            {
                Packet p = DrivePackets.Dequeue();
                switch ((PacketID)p.Data.ID)
                {
                    case PacketID.RPMAllDriveMotors:
                        MotorControl.SetAllRPM((sbyte)p.Data.Payload[0]);
                        break;
                    case PacketID.RPMFrontRight:
                    case PacketID.RPMFrontLeft:
                    case PacketID.RPMBackRight:
                    case PacketID.RPMBackLeft:
                        int MotorID = p.Data.ID - (byte)PacketID.RPMFrontRight;
                        MotorControl.SetRPM(MotorID, (sbyte)p.Data.Payload[1]);
                        break;
                    case PacketID.RPMSteeringMotor:
                        float SteerSpeed = UtilData.ToFloat(p.Data.Payload);
                        //MotorControl.SetSteerSpeed(SteerSpeed);
                        break;
                    case PacketID.SteerPosition:
                        float Position = UtilData.ToFloat(p.Data.Payload);
                        //MotorControl.SetRackAndPinionPosition(Position);
                        break;
                    case PacketID.SpeedAllDriveMotors:
                        float Speed = UtilData.ToFloat(p.Data.Payload);
                        MotorControl.SetAllSpeed(Speed);
                        break;
                    case PacketID.BaseSpeed:
                    case PacketID.ShoulderSpeed:
                    case PacketID.ElbowSpeed:
                    case PacketID.WristSpeed:
                    case PacketID.DifferentialVert:
                    case PacketID.DifferentialRotate:
                    case PacketID.HandGrip:
                        byte address = (byte)(p.Data.ID - 0x8A);
                        byte direction = 0x00;
                        if (p.Data.Payload[0] > 0)
                        {
                            direction = 0x01;
                        }
                        UtilCan.SpeedDir(CANBBB.CANBus0, false, 0x02, address, p.Data.Payload[1], direction);
                        break;
                }
            }
        }

        public static void ProcessPathPackets()
        {
            for (int i = 0; !PathPackets.IsEmpty() && i < NUM_PACKETS_TO_PROCESS; i++)
            {
                Packet p = PathPackets.Dequeue();
                switch ((PacketID)p.Data.ID)
                {
                    // TODO Maybe: Combine pathing speed and turn in same packet???
                    case PacketID.PathingSpeed:
                        PathSpeed = UtilData.ToFloat(p.Data.Payload);
                        MotorControl.SkidSteerDriveSpeed(PathSpeed, PathAngle);
                        break;
                    case PacketID.PathingTurnAngle:
                        PathAngle = UtilData.ToFloat(p.Data.Payload);
                        MotorControl.SkidSteerDriveSpeed(PathSpeed, PathAngle);
                        break;
                }
            }
        }

        public static void SendSensorData(int count)
        {
            foreach (ISensor Sensor in Sensors)
            {
                if (Sensor is MTK3339)
                {
                    var Tup = ((MTK3339)Sensor).GetCoords();
                    float Lat = Tup.Item1;
                    float Long = Tup.Item2;
                    Packet Pack = new Packet((byte)PacketID.DataGPS, true);
                    Pack.AppendData(UtilData.ToBytes(Lat));
                    Pack.AppendData(UtilData.ToBytes(Long));
                    Client.SendNow(Pack);
                    if(count == 100)
                    {
                        Packet HeadingFromGPSPack = new Packet((byte)PacketID.HeadingFromGPS, true);
                        //Math between two coords given from Tup and previousCoords
                        float latDiff = Lat - previousCoords.Item1;
                        float longDiff = Long - previousCoords.Item1;
                        float theta = (float)Math.Atan2(latDiff, longDiff);
                        if (longDiff > 0)
                        {
                            theta = 90 - theta;
                        }
                        else if(longDiff < 0)
                        {
                            theta = 270 -theta;
                        }
                        HeadingFromGPSPack.AppendData(UtilData.ToBytes(theta));
                        Client.SendNow(HeadingFromGPSPack);
                        previousCoords = Tup;
                    }
                }
                if (Sensor is BNO055)
                {
                    var Tup = ((BNO055)Sensor).GetVector(BNO055.VectorType.VECTOR_MAGNETOMETER);
                    float X = Tup.Item1;
                    float Y = Tup.Item2;
                    float Z = Tup.Item3;
                    Packet Pack = new Packet((byte)PacketID.DataMagnetometer, true);
                    Pack.AppendData(UtilData.ToBytes(X));
                    Pack.AppendData(UtilData.ToBytes(Y));
                    Pack.AppendData(UtilData.ToBytes(Z));
                    Client.SendNow(Pack);
                }
            }
        }

        public static void followTennisBall(UdpClient udpServer, IPEndPoint remoteEP, PID directionPID, PID distancePID)
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
            int realRightSpeed = 0;
            int realLeftSpeed = 0;
            if (!String.IsNullOrEmpty(dataString))
            {

                // Split the String into distance and direction
                // Is in format "Distance,Direction"
                // Distance is from 1 to 150 ish
                // Direction is from -10 to 10 degrees
                String[] speedDis = dataString.Split(',');
                Double distance = Convert.ToDouble(speedDis[0]);
                Double direction = (Convert.ToDouble(speedDis[1]));

                Double inverse_distance = 30 - distance; 
                if (inverse_distance < 0)
                {
                    inverse_distance = 0;
                }

                // If mode is set to not turn only the run
                if (!turnMode)
                {
                    distancePID.Feed(inverse_distance);
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

                realLeftSpeed = -(int)rightWheel;
                realRightSpeed = -(int)leftWheel;

                if (realLeftSpeed > 45)
                {
                    realLeftSpeed = 45;
                }
                else if (realLeftSpeed < -45)
                {
                    realLeftSpeed = -45;
                }

                if (realRightSpeed > 45)
                {
                    realRightSpeed = 45;
                }
                else if (realRightSpeed < -45)
                {
                    realRightSpeed = -45;
                }

                if (distance > 30)
                {
                    realLeftSpeed = 0;
                    realRightSpeed = 0;
                }


                Console.WriteLine("             Distance: " + distance + ", Turning: "+ direction);
                Console.WriteLine("Leftwheel: " + realLeftSpeed + ", Rightwheel: " + realRightSpeed);
                Console.WriteLine("Leftwheel: " + (sbyte)realLeftSpeed + ", Rightwheel: " + (sbyte)realRightSpeed);
            }
            // If data received is not a full string, stop motors.
            else
            {
                leftWheel = 0;
                rightWheel = 0;
                Console.WriteLine("no values recieved");
            }

            
            MotorControl.SetRPM(0, (sbyte)realRightSpeed);
            MotorControl.SetRPM(2, (sbyte)realRightSpeed);

            MotorControl.SetRPM(1, (sbyte)realLeftSpeed);
            MotorControl.SetRPM(3, (sbyte)-realLeftSpeed);
            

            //Motor[0].SetSpeed((float)(rightWheel));
            //Motor[1].SetSpeed((float)(leftWheel));

        }

        public static void Main(string[] args)
        {
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

            double SetTurn = 0;
            double PTurn = 1;
            double ITurn = 0;
            double DTurn = 0;

            double SetDis = 1;
            double PDis = 1;
            double IDis = 0;
            double DDis = 0;

            UdpClient udpServer = new UdpClient(9000);
            IPEndPoint remoteEP = new IPEndPoint(IPAddress.Any, 9000);
            /*
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
            */
            PID directionPID = new PID(PTurn, ITurn, DTurn);
            directionPID.SetTarget(SetTurn);
            PID distancePID = new PID(PDis, IDis, DDis);
            directionPID.SetTarget(SetDis);
            
            Quit = false;
            InitBeagleBone();
            //SetupClient();
            MotorControl.Initialize();
            MotorBoards.Initialize(CANBBB.CANBus0);
            int count = 0;
            do
            {
                //SendSensorData(count);
                //ProcessInstructions();
                followTennisBall(udpServer, remoteEP, directionPID, distancePID);
                Thread.Sleep(50);
                count++;
                if(count == 101)
                {
                    count = 0;
                }
            } while (!Quit);
        }
    }
}
