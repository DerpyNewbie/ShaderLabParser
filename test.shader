// NOTE(derpy): Shaderであることの宣言と、そのShader名の指定 (Material上で選択するときに表示される名前)
Shader "Unlit/NewUnlitShader"
{
    // NOTE(derpy): Materialで扱い、Shaderへ流す変数の定義
    Properties
    {
        /*
        NOTE(derpy):
            _MainTex: Shader内で使う変数名
            ("Texture", 2D): "Texture" の名前で表示し、型は Texture2D である
            "white": None の場合に送られてくるのは白テクスチャである
        */
        _MainTex ("Texture", 2D) = "white" {}
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 100

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            // フォグを使用します
            // #pragma multi_compile_fog

            // NOTE(derpy): UnityCG.cginc でない別個で定義したファイルを読む
            // #include "UnityCG.cginc"

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
            };

            struct v2f
            {
                float2 uv : TEXCOORD0;
                UNITY_FOG_COORDS(1)
                float4 vertex : SV_POSITION;
            };

            sampler2D _MainTex;
            float4 _MainTex_ST;

            v2f vert (appdata v)
            {
                v2f o;
                o.vertex = UnityObjectToClipPos(v.vertex);
                o.uv = TRANSFORM_TEX(v.uv, _MainTex);
                // NOTE(derpy): まだサポートしない
                // UNITY_TRANSFER_FOG(o,o.vertex);
                return o;
            }

            fixed4 frag (v2f i) : SV_Target
            {
                // テクスチャをサンプリング
                fixed4 col = tex2D(_MainTex, i.uv);
                // フォグを適用 NOTE(derpy): まだサポートしない
                // UNITY_APPLY_FOG(i.fogCoord, col);
                return col;
            }
            ENDCG
        }
    }
}