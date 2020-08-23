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

        public List<PipelineState> Parse(string filename)
        {
            List<PipelineState> result = new List<PipelineState>();

            var tocPath = Path.Combine(dataDir_, filename);

            var document = XDocument.Load(tocPath);

            var items = document.Descendants().Where(d => d.Name.LocalName == "PipelineState").ToList();

            foreach (var item in items)
            {
                var pipelineState = new PipelineState();

                pipelineState.name_ = item.Attribute("name").Value;

                // Root signature
                var rs = item.Descendants().Where(d => d.Name.LocalName == "RootSignature").SingleOrDefault();

                ParseRootSignature(rs, pipelineState);

                // Vertex shader
                var vs = item.Descendants().Where(d => d.Name.LocalName == "VertexShader").SingleOrDefault();

                ParseShader(vs, ShaderType.VertexShader, pipelineState);

                // Pixel shader
                var ps = item.Descendants().Where(d => d.Name.LocalName == "PixelShader").SingleOrDefault();
                ParseShader(ps, ShaderType.PixelShader, pipelineState);

                result.Add(pipelineState);
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

            var tempFile = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName());

            using (StreamWriter writer = new StreamWriter(tempFile))
            {
                writer.Write(builder.ToString());
            }

            return tempFile;
        }

        private void ParseRootSignature(XElement rs, PipelineState pipelineState)
        {
            if (rs == null)
                throw new Exception("RootSignature must be specified");
            else
            {
                var path = Path.Combine(dataDir_, rs.Attribute("path").Value);

                // root signatures are mandatory
                if (!File.Exists(path))
                    throw new System.IO.FileNotFoundException(path);

                ShaderComponent component = new ShaderComponent();

                component.type_ = ShaderType.RootSignature;
                component.path_ = GenerateRootShaderDummy(path);

                // root signatures can be shared across shaders, so rely on path to add them to
                if (RootSignatures.signatures_.ContainsKey(path))
                {
                    pipelineState.components_.Add(ShaderType.RootSignature, RootSignatures.signatures_[path]);
                }
                else
                {
                    RootSignatures.signatures_.Add(path, component);
                    pipelineState.components_.Add(ShaderType.RootSignature, component);
                }
            }
        }

        private void ParseShader(XElement shader, ShaderType type, PipelineState pipelineState)
        {
            if (shader != null)
            {
                var path = Path.Combine(dataDir_, shader.Attribute("path").Value);

                if (!File.Exists(path))
                    throw new FileNotFoundException(path);

                var entry = shader.Attribute("entry").Value;

                if (entry == null)
                    throw new Exception("Entry point must be specified");

                var component = new ShaderComponent();

                component.path_ = path;
                component.type_ = type;
                component.entry_ = entry;

                pipelineState.components_.Add(type, component);
            }
        }

        #endregion Private Methods
    }
}