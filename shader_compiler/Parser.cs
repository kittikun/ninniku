using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Xml.Linq;

namespace shader_compiler
{
    public class Parser
    {
        private string dataDir_;

        #region Public Methods

        public Parser(string dataPath)
        {
            dataDir_ = dataPath;
        }

        public void Parse(string filename)
        {
            var tocPath = Path.Combine(dataDir_, filename);

            var document = XDocument.Load(tocPath);

            var item = document.Descendants().Where(d => d.Name.LocalName == "PipelineState").SingleOrDefault();

            if (item != null)
            {
                var rs = item.Descendants().Where(d => d.Name.LocalName == "RootSignature").SingleOrDefault();

                var rsPath = Path.Combine(dataDir_, rs.Attribute("path").Value);

                if (!File.Exists(rsPath))
                    throw new System.IO.FileNotFoundException(rsPath);

                GenerateRootShaderDummy(rsPath);
            }
        }

        #endregion Public Methods

        #region Private Methods

        private void GenerateRootShaderDummy(string path)
        {
            List<string> lines = File.ReadAllLines(path).ToList();
            List<string> cleaned = new List<string>();
            List<string> formatted = new List<string>();

            foreach (var line in lines)
                if (line.Length > 0)
                    cleaned.Add(line);

            for (var i = 0; i < cleaned.Count; ++i)
            {
                if (i == (cleaned.Count - 1))
                    formatted.Add(string.Format("\"{0}\"", cleaned[i]));
                else
                    formatted.Add(string.Format("\"{0}\" \\", cleaned[i]));
            }

            var builder = new StringBuilder();

            builder.Append("#define RS \"");

            foreach (var str in formatted)
                builder.Append(string.Format("{0}\n", str));

            builder.Append(
@"
[numthreads(1, 1, 1)]
void main(uint3 DTI : SV_DispatchThreadID)
{
}");

            var tempFile = Path.Combine(System.IO.Path.GetTempPath(), Path.GetRandomFileName());

            using (StreamWriter writer = new StreamWriter(tempFile))
            {
                writer.Write(builder.ToString());
            }

            File.Delete(tempFile);

            Console.WriteLine(builder.ToString());
        }

        #endregion Private Methods
    }
}