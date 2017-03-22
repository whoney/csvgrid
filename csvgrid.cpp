#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <limits>
#include <cmath>

using namespace std;

typedef float csv_float; // replace with double if needed
#define csv_stof stof // replace with stod or stold for double

csv_float output_default_value = 0.0; // numeric_limits<csv_float>::min();

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

int source_lat_column;
int source_lon_column;
vector<int> source_value_columns;
int numValues;

int grid_width;
int grid_height;

csv_float length_scale = 2.0;

GridCell *cells;

GridCell *getCell (int gridX, int gridY) {
 
    if (gridX >= 0 && gridX < grid_width && gridY >= 0 && gridY < grid_height)
        return &(cells[gridY * grid_height + gridX]);
    else
        return NULL;
    
}

void barnes_interpolation (csv_float convergence_factor) {

    int gridSearchRadiusX = length_scale / lat_step;
    int gridSearchRadiusY = length_scale / lon_step;
    
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
                
                csv_float targetLat = lat_start + (gridX * lat_step);
                csv_float targetLon = lon_start + (gridY * lon_step);
                
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
                                
                                csv_float lon_distance = sourcePoint.lon - targetLon;
                                csv_float lat_distance = abs (sourcePoint.lat - targetLat);
                                if (lat_distance > 180.0)
                                    lat_distance = 360.0 - lat_distance; 
                                
                                csv_float dist_squared = lon_distance * lon_distance + lat_distance * lat_distance;
                                csv_float weight = exp (-dist_squared / (length_scale * length_scale * convergence_factor));
                                
                                //cout << "targetLat " << targetLat << " targetLon " << targetLon << " sourceLat" << sourcePoint.lat << " sourceLon " << sourcePoint.lon << endl;
                                
                                //cout << "lonD " << lon_distance << " latD " << lat_distance << " exp: " << (-dist_squared / (length_scale * length_scale * convergence_factor)) << " weight: " << weight << endl;
                                
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

int main (int argc, char *argv[]) {
    
    if (argc != 14) {
     
        cerr << "Use: " 
            << string(argv[0]) 
            << " <input csv>"
            << " <lat start>"
            << " <lat end>"
            << " <lon start>"
            << " <lon end>"
            << " <lat step>"
            << " <lon step>"
            << " <source lat column>"
            << " <source lon column>"
            << " <source data columns like 2,3,4>"
            << " <length scale>"
            << " <passes>"
            << " <output csv>"
            << endl;
            
        cerr << '\t' << "column numbers start from zero" << endl;
        cerr << '\t' << "passes are comma-separated floating values like 1.0,0.3,0.2" << endl;
            
        return 1;
        
    }
    
    lat_start = csv_stof (string (argv[2]));
    lat_end = csv_stof (string (argv[3]));
    lon_start = csv_stof (string (argv[4]));
    lon_end = csv_stof (string (argv[5]));
    lat_step = csv_stof (string (argv[6]));
    lon_step = csv_stof (string (argv[7]));
    source_lat_column = stoi (string (argv[8]));
    source_lon_column = stoi (string (argv[9]));
    
    vector<string> source_value_columns_s;
    split_string (string (argv[10]), ',', source_value_columns_s);
    for (const string& s: source_value_columns_s) {
        source_value_columns.push_back (stoi (s));
    }
    
    numValues = source_value_columns.size ();
    
    length_scale = csv_stof (string (argv[11]));
    
    vector<string> passes_strings;
    vector<csv_float> passes;
    split_string (string (argv[12]), ',', passes_strings);
    for (const string& s: passes_strings)
        passes.push_back (csv_stof (s));
    
    grid_width = (int) ((lat_end - lat_start) / lat_step);
    grid_height = (int) ((lon_end - lon_start) / lon_step);
    
    cout << "Grid size: " << grid_width << "x" << grid_height << " (" << (grid_width * grid_height) << " points), length scale " << length_scale << endl;
    
    ifstream ifs;
    
    ifs.open (string (argv[1]));
    if (!ifs.good ()) {
        cerr << "Failed to open " << string (argv[1]) << endl;
        return 1;
    }
    
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
    
    int source_point_count = 0;
    vector<string> tokens;
    
    for (string line; getline(ifs, line);) {
        split_string (line, ',', tokens);
        
        if (source_lat_column < tokens.size () && source_lon_column < tokens.size ()) {
            csv_float source_lat = csv_stof (tokens[source_lat_column]);
            csv_float source_lon = csv_stof (tokens[source_lat_column]);
            
            int cell_x = (int) ((source_lat - lat_start) / lat_step);
            int cell_y = (int) ((source_lon - lon_start) / lon_step);
            
            GridCell *cell = getCell (cell_x, cell_y);
            
            if (cell) {

                SourcePoint p;
                p.lat = source_lat;
                p.lon = source_lon;
                for (int i: source_value_columns) 
                    p.values.push_back (csv_stof (tokens[i]));

                cell -> sourcePoints.push_back (p);
                
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
    ofs.open (string (argv[13]));
    
    for (int gridY = 0; gridY < grid_height; gridY ++) {
        for (int gridX = 0; gridX < grid_width; gridX ++) {
            
            GridCell *targetCell = getCell (gridX, gridY);

            if (targetCell) {
                
                csv_float targetLat = gridX * lat_step;
                csv_float targetLon = gridY * lon_step;
                
                ofs << targetLat << "," << targetLon;
                
                for (int i = 0; i < numValues; i++) 
                    ofs << "," << (targetCell -> hasValues ? targetCell -> outputValues[i] : output_default_value);                    
                    
                ofs << '\n';
                
            }
            
        }
    }
    
    
    ofs.close ();
    
    cout << "Done" << endl;
    
	return 0;
}

