#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <limits>
#include <cmath>
#include <algorithm>

using namespace std;

typedef float csv_float; // replace with double if needed
#define csv_stof stof // replace with stod or stold for double

csv_float output_default_value = 0.0; // numeric_limits<csv_float>::min();

const int source_tag_column1 = 2;
const int source_tag_column2 = 3;

const int source_lon_column = 4;        // longitude -180..180
const int source_lat_column = 5;        // latitude -90..90
const int source_value_column = 6;

struct SourcePoint {
 
    csv_float lat;
    csv_float lon;
    vector<csv_float> values;
    
};

struct GridCell {

    bool hasValues;
    vector<csv_float> outputValues;
    vector<csv_float> prevOutputValues;
    vector<SourcePoint> sourcePoints;
    
};

csv_float lat_start;
csv_float lat_end;
csv_float lon_start;
csv_float lon_end;
csv_float lat_step;
csv_float lon_step;

vector<pair <string, string>> source_tags;
int numValues;

int grid_width;
int grid_height;

csv_float length_scale = 2.0;

GridCell *cells;

GridCell *getCell (int gridX, int gridY) {
 
    if (gridX >= 0 && gridX < grid_width && gridY >= 0 && gridY < grid_height)
        return &(cells[gridY * grid_width + gridX]);
    else
        return NULL;
    
}

GridCell *getCellByLatLon (csv_float lat, csv_float lon) {
    
    int cell_x = (int) ((lon - lon_start) / lon_step);
    int cell_y = (int) ((lat - lat_start) / lat_step);
    
    return getCell (cell_x, cell_y);
    
}

void barnes_interpolation (csv_float convergence_factor) {

    int gridSearchRadiusX = ceil (length_scale / lon_step);
    int gridSearchRadiusY = ceil (length_scale / lat_step);
    
    vector<csv_float> influence_sums (numValues);

    for (int gridY = 0; gridY < grid_height; gridY ++) {
        for (int gridX = 0; gridX < grid_width; gridX ++) {
            GridCell *targetCell = getCell (gridX, gridY);
            if (targetCell) {
                for (int i = 0; i < numValues; i++) 
                    targetCell -> prevOutputValues[i] = targetCell -> outputValues[i];
            }
        }
    }

    
    for (int gridY = 0; gridY < grid_height; gridY ++) {
        for (int gridX = 0; gridX < grid_width; gridX ++) {

            GridCell *targetCell = getCell (gridX, gridY);

            if (targetCell) {
                
                csv_float targetLon = lon_start + (gridX * lon_step);
                csv_float targetLat = lat_start + (gridY * lat_step);
                
                csv_float weightSum = 0.0;
                for (int i = 0; i < numValues; i++)
                    influence_sums[i] = 0.0;
                
                int pointCount = 0;
                
                for (int gridDy = -gridSearchRadiusY; gridDy <= gridSearchRadiusY; gridDy ++) {
                    for (int gridDx = -gridSearchRadiusX; gridDx <= gridSearchRadiusX; gridDx ++) {
                        
                        int lookY = gridY + gridDy;
                        int lookX = gridX + gridDx;
                        
                        if (lookX < 0)
                            lookX += grid_width;
                        else if (lookX >= grid_width)
                            lookX -= grid_width;
                        
                        GridCell *lookCell = getCell (lookX, lookY);
                        if (lookCell) {
                            for (const SourcePoint& sourcePoint: lookCell -> sourcePoints) {
                                
                                csv_float lon_distance = abs (sourcePoint.lon - targetLon);
                                csv_float lat_distance = abs (sourcePoint.lat - targetLat);
                                if (lon_distance > 180.0)
                                    lon_distance = 360.0 - lon_distance; 
                                
                                csv_float dist_squared = lon_distance * lon_distance + lat_distance * lat_distance;
                                csv_float weight = exp (-dist_squared / (length_scale * length_scale * convergence_factor));
                                
                                for (int i = 0; i < numValues; i++) 
                                    influence_sums[i] += (sourcePoint.values[i] - lookCell -> prevOutputValues[i]) * weight;
                                
                                weightSum += weight;
                                pointCount ++;
                            }
                        }
                    }
                    
                }
            
                for (int i = 0; i < numValues; i++) {
                    targetCell -> outputValues[i] = targetCell -> prevOutputValues[i] + (influence_sums[i] / weightSum);
                }
                
                if (pointCount)
                    targetCell -> hasValues = true;
                
            }
            
        }
    }
    
}

void split_string (const string& str, const char delim, vector<string>& tokens) {
    size_t prev = 0, pos = 0;
    tokens.clear ();
    do {
        pos = str.find(delim, prev);
        if (pos == string::npos) pos = str.length();
        string token = str.substr(prev, pos-prev);
        if (!token.empty()) tokens.push_back(token);
        prev = pos + 1;
    }
    while (pos < str.length() && prev < str.length());
}

void scan_tags (ifstream& ifs) {
 
    vector<string> tokens;
    pair<string,string> last_tag;
    int last_value_index = -1;
    
    for (string line; getline(ifs, line);) {
        split_string (line, ',', tokens);
        if (source_tag_column1 < tokens.size () && source_tag_column2 < tokens.size ()) {
            
            pair<string,string> tag (tokens[source_tag_column1], tokens[source_tag_column2]);
            
            if (tag != last_tag) {
                last_value_index = -1;
                for (int i = 0; i < source_tags.size (); i++) {
                    if (source_tags[i] == tag) {
                        last_value_index = i;
                        break;
                    }
                }
                
                if (last_value_index == -1) {
                    last_value_index = source_tags.size ();
                    source_tags.push_back (tag);
                }
                
                last_tag = tag;
                
            }
        }
    }
    
    for (auto& tag: source_tags) {
        cout << "Found tag: " << tag.first << ", " << tag.second << endl;
    }
    
    numValues = source_tags.size ();
    
}

int main (int argc, char *argv[]) {
    
    if (argc != 11) {
     
        cerr << "Use: " 
            << string(argv[0]) 
            << " <input csv>"
            << " <lat start>"
            << " <lat end>"
            << " <lon start>"
            << " <lon end>"
            << " <lat step>"
            << " <lon step>"
            << " <length scale>"
            << " <passes>"
            << " <output csv>"
            << endl;
            
        cerr << '\t' << "passes are comma-separated floating values like 1.0,0.3,0.2" << endl;
            
        return 1;
        
    }
    
    lat_start = csv_stof (string (argv[2]));
    lat_end = csv_stof (string (argv[3]));
    lon_start = csv_stof (string (argv[4]));
    lon_end = csv_stof (string (argv[5]));
    lat_step = csv_stof (string (argv[6]));
    lon_step = csv_stof (string (argv[7]));

    length_scale = csv_stof (string (argv[8]));
    
    vector<string> passes_strings;
    vector<csv_float> passes;
    split_string (string (argv[9]), ',', passes_strings);
    for (const string& s: passes_strings)
        passes.push_back (csv_stof (s));
    
    grid_width = (int) ((lon_end - lon_start) / lon_step);
    grid_height = (int) ((lat_end - lat_start) / lat_step);
    
    cout << "Grid size: " << grid_width << "x" << grid_height << " (" << (grid_width * grid_height) << " points), length scale " << length_scale << endl;
    
    ifstream ifs;
    
    ifs.open (string (argv[1]));
    if (!ifs.good ()) {
        cerr << "Failed to open " << string (argv[1]) << endl;
        return 1;
    }
    
    scan_tags (ifs);
    
    ifs.clear();
    ifs.seekg(0);
    
    cells = new GridCell[grid_width * grid_height];
    
    for (int i = 0; i < grid_width * grid_height; i++) {
        cells[i].hasValues = false;
        cells[i].outputValues.resize (numValues);
        cells[i].prevOutputValues.resize (numValues);
        for (int j = 0; j < numValues; j++) {
            cells[i].outputValues[j] = 0.0; 
            cells[i].prevOutputValues[j] = 0.0;
        }
    }
    
    bool first_line = true;
    vector<string> tokens_template;
    
    int source_point_count = 0;
    vector<string> tokens;
    pair<string,string> last_tag;
    int last_value_index = -1;
    
    for (string line; getline(ifs, line);) {
        split_string (line, ',', tokens);
        
        if (source_lat_column < tokens.size () && source_lon_column < tokens.size ()) {
            
            if (first_line) {
                for (const string& s: tokens)
                    tokens_template.push_back (s);
                first_line = false;
            }
            
            pair<string,string> tag (tokens[source_tag_column1], tokens[source_tag_column2]);
            
            if (tag != last_tag || last_value_index == -1) {
                last_value_index = -1;
                for (int i = 0; i < source_tags.size (); i++) {
                    if (source_tags[i] == tag) {
                        last_value_index = i;
                        break;
                    }
                }
                last_tag = tag;
            }
            
            if (last_value_index == -1) {
                cerr << "Unexpected tag" << endl;
                return 1;
            }
            
            csv_float source_lat = csv_stof (tokens[source_lat_column]);
            csv_float source_lon = csv_stof (tokens[source_lon_column]);
            
            GridCell *cell = getCellByLatLon (source_lat, source_lon);
            
            if (cell) {
                
                csv_float value = csv_stof (tokens[source_value_column]);
                
                bool point_found = false;
                for (SourcePoint& p: cell -> sourcePoints) {
                    if (p.lat == source_lat && p.lon == source_lon) {
                        p.values[last_value_index] = value;
                        point_found = true;
                        break;
                    }
                }
                
                if (!point_found) {
                    SourcePoint p;
                    p.lat = source_lat;
                    p.lon = source_lon;
                    p.values.resize (numValues);
                    p.values[last_value_index] = value;
                    cell -> sourcePoints.push_back (p);
                }
                
            }
            
            source_point_count ++;
        }
        
    }
    
    ifs.close ();
    
    cout << "Source points: " << source_point_count << endl;

    for (csv_float convergence_factor: passes) {
        cout << "Pass: convergence factor " << convergence_factor << endl;
        barnes_interpolation (convergence_factor);
    }

    cout << "Writing output..." << endl;
    
    ofstream ofs;
    ofs.open (string (argv[10]));
    vector<string> output_tokens (tokens_template);
    int value_index = 0;
    
    for (const auto& tag: source_tags) {
        
        output_tokens[source_tag_column1] = tag.first;
        output_tokens[source_tag_column2] = tag.second;

        for (int gridY = 0; gridY < grid_height; gridY ++) {
            for (int gridX = 0; gridX < grid_width; gridX ++) {
                
                GridCell *targetCell = getCell (gridX, gridY);

                if (targetCell) {
                    
                    csv_float targetLon = lon_start + gridX * lon_step;
                    csv_float targetLat = lat_start + gridY * lat_step;
                    
                    int column_index = 0;
                    for (const string& s: output_tokens) {
                        if (column_index != 0)
                            ofs << ",";
                        if (column_index == source_lat_column)
                            ofs << targetLat;
                        else if (column_index == source_lon_column)
                            ofs << targetLon;
                        else if (column_index == source_value_column)
                            ofs << (targetCell -> hasValues ? targetCell -> outputValues[value_index] : output_default_value);
                        else
                            ofs << s;
                        column_index ++;
                    }
                    
                    ofs << '\n';
                    
                }
                
            }
        }
        
        value_index ++;
        
    }
    
    ofs.close ();
    
    cout << "Done" << endl;
    
	return 0;
}

