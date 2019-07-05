# Computer Animation Competition (Softwaredokumentation)

## Gruppenmitglieder

- Patrick Hähn
- Sebastian Kühn
- Volker Sobek

## Künstlerisches Konzept

## Technisches Konzept

## Arbeitsschritte + Teilergebnisse

### Terrain Generation

`
class terrain
{
	float * get_heights(float range, float rigidity);
	void build();
	void clamp_heights();
	int start_frame = 0;
	int current_frame = 0;
	int frame_loc;
	int max_frame_loc;
	static int stone_loc;
	static int grass_loc;
	static int snow_loc;
public:
	terrain(float size, int resolution, int start_frame, int max_frame, int shader_program, std::string stone, std::string grass, std::string snow);
	~terrain();
	geometry terra;
	float * heights;
	int frames;
	float size;
	int resolution;
	const glm::vec4 hill_color = glm::vec4(0.7,0.7,0.7,1.0);
	const glm::vec4 mountain_color = glm::vec4(0.5,0.5,0.5, 1.0);
	const glm::vec4 snow_color = glm::vec4(1.0,1.0,1.0, 1.0);
	const glm::vec4 ground_color = glm::vec4(0.2,0.5,0.2, 1.0);
	float min_height = 0.0;
	float max_height = 1.0;
	float lowest_height = 0.0;
	float highest_height = 1.0;
	glm::vec3 * get_normal_at_pos(float x, float z);
	void get_frame_locations(int shader_program);
	void set_frames(int start, int max);
	void reset_current_frame();
	void increase_current_frame(int increase = 1);
	void render(int model_loc);
	static void get_texture_locations(int shader_program);
	static void load_textures(std::string stone, std::string grass, std::string snow);
	static unsigned create_texture_rgba32f(int width, int height, float* data);
	static float* load_texture_data(std::string filename, int* width, int* height);
	static void set_texture_filter_mode(unsigned int texture, GLenum mode);
	static void set_texture_wrap_mode(unsigned int texture, GLenum mode);
};
`


## Ergebnis

## Kommentare

## Referenzen