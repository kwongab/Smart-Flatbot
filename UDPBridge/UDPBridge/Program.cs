using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace UDPBridge
{
    class Program
    {
        static void Main(string[] args)
        {

            UdpClient udpServer = new UdpClient(8000); // UDP Port from RaspberryPi
            IPEndPoint remoteEP = new IPEndPoint(IPAddress.Any, 8000); 

            var client = new UdpClient();
            // IP and port for Rover Beaglebone
            IPEndPoint ep = new IPEndPoint(IPAddress.Parse("192.168.0.51"), 9000); 
            client.Connect(ep);

            Byte[] sendBytes;


            Console.WriteLine("Loop Started");
            while (true)
            {

                var data = udpServer.Receive(ref remoteEP);
                var stringData = Encoding.ASCII.GetString(data);
                Console.WriteLine(stringData);

                sendBytes = Encoding.ASCII.GetBytes(stringData);

                client.Send(sendBytes, sendBytes.Length);


            }
        }
    }
}
