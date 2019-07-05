# Computer Animation Competition (Softwaredokumentation)

## Gruppenmitglieder

- Patrick Hähn
- Sebastian Kühn
- Volker Sobek

## Künstlerisches Konzept

## Technisches Konzept

## Arbeitsschritte + Teilergebnisse

## Noise Generation

### Terrain Generation

Die Generierung der Terrains sowie dessen Visualisierung werden in einer Klasse **terrain** gekapselt, sodass nach deren Instanziierung lediglich die öffentliche Funktion **terrain::render(int)** in der Render-Loop aufgerufen werden muss. Die Klasse bietet desweiteren eine Funktion **terrain::get_normal_at_pos(float,float)**, die für die physikalischen Berechnungen benötigt wird. Die Generierung des Terrains basiert auf der obigen eigenen Implementierung von **perlin_noise** und wurde inspiriert durch *Red Blob Games*[1].

```cpp
class terrain
{
	// Private member properties
	int start_frame = 0;
	int current_frame = 0;
	int frame_loc;
	int max_frame_loc;
	geometry terra;
	float * heights;
	int frames;
	float size;
	int resolution;
	float min_height = 0.0;
	float max_height = 1.0;
	float lowest_height = 0.0;
	float highest_height = 1.0;

	// Static private class properties
	static int stone_loc;
	static int grass_loc;
	static int snow_loc;

	// Private member functions
	float * get_heights(float range, float rigidity);
	void build();
	void clamp_heights();
	void get_frame_locations(int shader_program);
	void set_frames(int start, int max);
	void reset_current_frame();
	void increase_current_frame(int increase = 1);

	// Static private class functions
	static void get_texture_locations(int shader_program);
	static void load_textures(std::string stone, std::string grass, std::string snow);
	static unsigned create_texture_rgba32f(int width, int height, float* data);
	static float* load_texture_data(std::string filename, int* width, int* height);
	static void set_texture_filter_mode(unsigned int texture, GLenum mode);
	static void set_texture_wrap_mode(unsigned int texture, GLenum mode);

public:
	// Public constructor & destructor
	terrain(float size, int resolution, int start_frame, int max_frame, int shader_program, std::string stone, std::string grass, std::string snow);
	~terrain();

	// Public member functions
	glm::vec3 * get_normal_at_pos(float x, float z);
	void render(int model_loc);
};
```


## Ergebnis

## Kommentare

## Referenzen

[1] Red Blob Games, https://www.redblobgames.com/maps/terrain-from-noise/, abgerufen: 05.07.2019