using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace WpfTestingClient
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct ImageHeader
    {
        public UInt32 dwWidth;
        public UInt32 dwHeight;
    }

    public class ImageFrame
    {
        public ImageHeader Header;
        public byte[] pData;

        public static unsafe ImageFrame FromBytes(byte[] bytes)
        {
            var HdrSize = Marshal.SizeOf(typeof(ImageHeader));
            if (bytes.Length < HdrSize)
                throw new ArgumentException();

            ImageFrame ret = new ImageFrame();
            fixed (byte* pData = &bytes[0])
            {
                ret.Header = (ImageHeader)Marshal.PtrToStructure(new IntPtr(pData), typeof(ImageHeader));
            }

            var ImageDataSize = ret.Header.dwHeight * ret.Header.dwWidth;
            ret.pData = new byte[ImageDataSize];
            Array.Copy(bytes, HdrSize, ret.pData, 0, ImageDataSize);

            return ret;
        }
    }

}
