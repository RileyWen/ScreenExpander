using System;
using System.Diagnostics;
using System.IO;
using System.IO.Pipes;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Windows;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Threading;

namespace WpfTestingClient
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        // private NamedPipeClientStream m_pipe;

        public MainWindow()
        {
            InitializeComponent();
            // m_pipe = new NamedPipeClientStream(".", "RileyNamedPipe",
            //     PipeAccessRights.ReadData | PipeAccessRights.WriteAttributes,
            //     PipeOptions.None,
            //     System.Security.Principal.TokenImpersonationLevel.None,
            //     System.IO.HandleInheritability.None);

            //Screen.Dispatcher.BeginInvoke(imageUpdateDelegate);

            // DispatcherTimer timer = new DispatcherTimer
            // {
            //     Interval = TimeSpan.FromSeconds(1 / 60.0f) // 60Hz
            // };
            // timer.Tick += (o, e) =>
            // {
            //     DateTime dtCurrentTime = DateTime.Now;
            //     if (m_bitmap != null)
            //         Screen.Source = m_bitmap;
            // };
            // timer.Start();

            new Thread(new ThreadStart(ImageUpdateThread)).Start();

            Debug.WriteLine("Init finished.");
        }

        private struct ScreenImageFrame
        {
            public ushort wWidth;
            public ushort wHeight;
            public byte[] aRawPixelData;
        }

        private void ImageUpdateThread()
        {
            try
            {
                IPAddress ipAddress = IPAddress.Parse("192.168.116.128");
                IPEndPoint remoteEP = new IPEndPoint(ipAddress, 7888);

                Socket sock = new Socket(ipAddress.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

                try
                {
                    sock.Connect(remoteEP);
                    Debug.WriteLine("Socket connected to {0}", sock.RemoteEndPoint.ToString());

                    using (var stream = new NetworkStream(sock))
                    {
                        BinaryReader binaryReader = new BinaryReader(stream);

                        while (true)
                        {
                            var frame = new ScreenImageFrame();

                            try
                            {
                                frame.wWidth = binaryReader.ReadUInt16();
                                frame.wHeight = binaryReader.ReadUInt16();

                                frame.aRawPixelData = binaryReader.ReadBytes(frame.wWidth * frame.wHeight * 4);

                                BitmapSource bmSource = BitmapSource.Create(
                                    frame.wWidth, frame.wHeight, 96, 96,
                                    PixelFormats.Bgra32, null,
                                    frame.aRawPixelData, 4 * frame.wWidth);

                                bmSource.Freeze();

                                Screen.Dispatcher.Invoke(() =>
                                {
                                    Screen.Source = bmSource;
                                });
                            }
                            catch (Exception e)
                            {
                                Debug.WriteLine($"[{nameof(ImageUpdateThread)}] Expection occured while updating image: {e.Message}");
                                break;
                            }
                        }
                    }
                }
                catch (Exception e)
                {
                    Debug.WriteLine(e.ToString());
                }
                finally
                {
                    sock.Shutdown(SocketShutdown.Both);
                    sock.Close();
                }
            }
            catch (Exception e)
            {
                Debug.WriteLine(e.ToString());
            }

        }

        private void ImageUpdateThreadForTest()
        {
            Random random = new Random();

            while (true)
            {
                try
                {
                    BitmapSource bmSource = BitmapSource.Create(
                        2, 1, 1, 1,
                        PixelFormats.Bgra32, null,
                        new byte[] { (byte)random.Next(), (byte)random.Next(), (byte)random.Next(), 255, (byte)random.Next(), 200, 200, 255 }, 2 * 4);

                    bmSource.Freeze();

                    Screen.Dispatcher.Invoke(() =>
                    {
                        Screen.Source = bmSource;
                    });

                    // m_bitmap = bmSource;

                    Thread.Sleep(TimeSpan.FromSeconds(2));
                }
                catch (Exception e)
                {
                    Debug.WriteLine($"[{nameof(ImageUpdateThreadForTest)}] Expection occured: {e.Message}");
                    break;
                }
            }
            // BitmapImage bitmap = new itmapImage();

            // bitmap.BeginInit();
            // bitmap.UriSource = new Uri(@"http://static01.coloros.com/bbs/data/attachment/forum/201503/17/092752ywabeihuha0iza0d.jpg");
            // bitmap.EndInit();

            // if (bitmap.IsDownloading)
            // {
            //     bitmap.DownloadCompleted += (o, e) =>
            //     {
            //         bitmap.Freeze();
            //         m_bitmap = bitmap;
            //     };
            // }
            // else
            // {
            //     bitmap.Freeze();
            //     m_bitmap = bitmap;
            // }
            // Screen.Source = bitmap;

            // while (true) ;

            // byte[] ImageBytes = new byte[1920 * 1080 * 32 / 8 + 8];

            // try
            // {
            //     m_pipe.Connect();
            // }
            // finally { }

            // m_pipe.ReadMode = PipeTransmissionMode.Message;

            // int BytesRead = m_pipe.Read(ImageBytes, 0, ImageBytes.Length);

            // Debug.WriteLine("Receive %d bytes", BytesRead);
            // ImageFrame frame = ImageFrame.FromBytes(ImageBytes);

            // BitmapSource bmSource = BitmapSource.Create(
            //     (int)frame.Header.dwWidth, (int)frame.Header.dwHeight, 200, 200,
            //     PixelFormats.Bgra32, null,
            //     frame.pData, (int)frame.Header.dwWidth * 4);

        }

        private void Button_Connect_Click(object sender, RoutedEventArgs e)
        {
        }
    }
}