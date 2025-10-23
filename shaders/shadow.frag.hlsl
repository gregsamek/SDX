struct Fragment_Input
{
    float4 position_clipspace : SV_Position;
};

void main(void){}

/*

// if a platform won't accept `void` return, 
// use a depth-only pixel shader that writes SV_Depth (still zero color targets)

struct Fragment_Input { float4 position_clipspace : SV_Position; };
struct Fragment_Output { float depth : SV_Depth; };

Fragment_Output main(Fragment_Input fragment)
{
    Fragment_Output o;
    o.depth = fragment.position_clipspace.z / max(fragment.position_clipspace.w, 1e-7f);
    return o;
}

*/