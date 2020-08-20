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
        private const string rsTemplate = @"
[numthreads(1, 1, 1)]
void main(uint3 DTI : SV_DispatchThreadID)
{
}";

        private string dataDir_;
        private Options options_;

        #region Public Methods

        public Parser(string dataPath, Options o)
        {
            dataDir_ = dataPath;
            options_ = o;
        }

        public List<Shader> Parse(string filename)
        {
            List<Shader> result = new List<Shader>();

            var tocPath = Path.Combine(dataDir_, filename);

            var document = XDocument.Load(tocPath);

            var item = document.Descendants().Where(d => d.Name.LocalName == "PipelineState").SingleOrDefault();

            if (item != null)
            {
                // Root signature
                var rs = item.Descendants().Where(d => d.Name.LocalName == "RootSignature").SingleOrDefault();

                var rsPath = Path.Combine(dataDir_, rs.Attribute("path").Value);

                if (!File.Exists(rsPath))
                    throw new System.IO.FileNotFoundException(rsPath);

                Shader shader = new Shader();

                shader.name_ = rs.Attribute("name").Value;
                shader.type_ = ShaderType.RootSignature;
                shader.path_ = GenerateRootShaderDummy(rsPath);

                result.Add(shader);
            }

            return result;
        }

        #endregion Public Methods

        #region Private Methods

        private string GenerateRootShaderDummy(string path)
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
                    formatted.Add(string.Format("{0}", cleaned[i]));
                else
                    formatted.Add(string.Format("{0} \\", cleaned[i]));
            }

            var builder = new StringBuilder();

            builder.Append("#define RS ");

            foreach (var str in formatted)
                builder.Append(string.Format("{0}\n", str));

            builder.Append(rsTemplate);

            var tempFile = Path.Combine(options_.OutDir, Path.GetRandomFileName());

            using (StreamWriter writer = new StreamWriter(tempFile))
            {
                writer.Write(builder.ToString());
            }

            return tempFile;
        }

        #endregion Private Methods
    }
}