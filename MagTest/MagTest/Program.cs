using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Scarlet.Components.Sensors;
using Scarlet.IO.BeagleBone;
using Scarlet.Utilities;
using Scarlet.IO;

namespace MagTest
{
    class Program
    {
        static void Main(string[] args)
        {

            int DEGREES_OFFSET = -20;

            StateStore.Start("MagTest");
            BeagleBone.Initialize(SystemMode.DEFAULT, true);
            BBBPinManager.AddMappingsI2C(BBBPin.P9_17, BBBPin.P9_26);

            BBBPinManager.ApplyPinSettings(BBBPinManager.ApplicationMode.APPLY_IF_NONE);
            II2CBus I2C = I2CBBB.I2CBus1;
            Console.WriteLine("Status: BBB Pin Mapings Complete");

            if (I2C == null)
            {
                Console.WriteLine("ERROR: I2C not mapped correctly. Fix First");
            }
            else
            {
                Console.WriteLine("Status: I2C Not null");
                BNO055 magObj = new BNO055(I2C);
                Console.WriteLine("Status: BNO055 object created");

                magObj.SetMode(BNO055.OperationMode.OPERATION_MODE_COMPASS);
                magObj.Begin();

                double direction;

                while (true)
                {
                    Tuple<float, float, float> magnets;
                    magnets = magObj.GetVector(BNO055.VectorType.VECTOR_MAGNETOMETER);

                    // Convert magnetic readings to degrees
                    direction = getDegrees(magnets);

                    //Add the offset from specific location
                    direction += DEGREES_OFFSET;

                    // Make sure degrees value is between 0 and 360
                    direction = standarize(direction);

                    Console.WriteLine(direction);

                }

            }

        }

        public static double getDegrees(Tuple<float, float, float> magnets)
        {
            double direction;

            double xmag = (magnets.Item1);
            double ymag = (magnets.Item2);

            if (ymag > 0)
            {
                direction = 90 - Math.Atan(xmag / ymag) * (180 / (Math.PI));
            }
            else if (ymag < 0)
            {
                direction = 270 - Math.Atan(xmag / ymag) * (180 / (Math.PI));
            }
            else
            {
                if (xmag < 0)
                {
                    direction = 180;
                }
                else
                {
                    direction = 0;
                }
            }

            return direction;
        }

        public static double standarize(double direction)
        {
            if (direction < 0)
            {
                return direction + 360;
            }
            else if (direction > 360)
            {
                return direction - 360;
            }
            return direction;
        }
    }
}
