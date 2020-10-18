#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <queue>
#include <iostream>
#include <iomanip>
#include <iterator>
#include "time.h"
using namespace std;

struct node
{
    string parent_actor;
    string movie;
    int bacon_number;
};

void read_in_database( map <string, vector<string> >&, map <string, vector<string> >&, ifstream&);
void build_mst( map <string, node>&, map <string, vector<string> >&, map <string, vector<string> >&, map <string, int>&, map <string, int>&, string );
void display_longest_path (string, string, map <string,node>&);
void display_path (string, string, map <string,node>&);
void output_info( map <int, int>&, map <string, node>&, map <string, vector<string> >&, int, int, clock_t, clock_t, string );

int main( int argc, char *argv[] )
{
    string start_actor = "Kevin Bacon";
    string actor;
    ifstream fin;

    map <string, vector<string> > movies_to_actors;
    map <string, vector<string> > actors_to_movies;
    map <string, int> visited_movies, visited_actors;
    map <string, node> actor_info;
    map <int, int> frequency;
    map <string, node> :: iterator itr;
    map <string, vector<string> > :: iterator itr3;
    int total_actors = 0, total_movies = 0;
    clock_t t, s; 
   
    //checks amount of arguments, prints usage if not correct
    switch ( argc )
    {
	case 2:
	    break;
	case 3:
	    //sets starting actor to third argument if necessary
	    start_actor = argv[2];
	    break;
	default:
	    cout << "Usage: " << argv[0] << " info.dat \"Actor\"(Optional)" << endl;
	    return -1;
    }

    fin.open( argv[1] );

    if( !fin )
    {
	cout << "Error opening file " << argv[1] << endl;
	return -1;
    }

    t = clock();

    //reads in actors and movies from the file, putting them into maps
    read_in_database( movies_to_actors, actors_to_movies, fin );

    t = clock() - t;

    //reads through list of movies and creates a visited map for them, setting each as unvisited
    for( itr3 = movies_to_actors.begin(); itr3 != movies_to_actors.end(); ++itr3 )
    {
	visited_movies.insert( pair <string, int> (itr3->first, 0) );
	//counts total number of movies for later use
	total_movies++;
    }

    //reads through list of actors and creates a visited map for them, setting each as unvisited
    for( itr3 = actors_to_movies.begin(); itr3 != actors_to_movies.end(); ++itr3 )
    {
	visited_actors.insert( pair <string, int> (itr3->first, 0) );
	//counts total number of actors for later use
	total_actors++;
    }

    s = clock();

    //builds a mst with bacon numbers and paths
    build_mst( actor_info, movies_to_actors, actors_to_movies, visited_movies, visited_actors, start_actor );
    
    s = clock() - s;

    //creates a histogram of bacon numbers
    for( itr = actor_info.begin(); itr != actor_info.end(); ++itr )
    {
        if( frequency.find(itr->second.bacon_number) == frequency.end() )
	    frequency.insert( pair <int, int> (itr->second.bacon_number, 1) );
	else
	    frequency[itr->second.bacon_number]++;
    }

    //if the starting actor does not exist, exit program
    if( frequency.size() == 1 )
    {
	cout << "Could not find performer named " << start_actor << endl;
	return -1;
    }

    //outputs the necessary info
    output_info( frequency, actor_info, actors_to_movies, total_movies, total_actors, t, s, start_actor );

    actor = "";
    cout << "Enter actor (blank line to quit): ";
    getline(cin, actor);

    //runs until blank line is input
    while( actor != "" )
    {
	//displays the path from the input actor to the starting actor
	display_path( actor, start_actor, actor_info );
	cout << endl; 

	cout << "Enter actor (blank line to quit): ";
        getline(cin, actor);
    }


    return 0;
}

/********************************************************************/
// This function reads in data from the file and stores the
// data in an adjacency-list format.
//
// algorithm : 
// 1. read a whole line from the file
// 2. parse the string for the movie name and store in movie variable.
// 3. then, parse the rest of the string for names of actors which
//    are each separated by a pipe '|'.
// 4. as we parse actors and movies, from a single line in the file,
//	  we insert them into two maps, creating the adjacency lists.
// 5. if we are not at the end of the file, we goto 1., otherwise
//    we return from this function.
/********************************************************************/
void read_in_database( map <string, vector<string> >& movies_to_actors, map <string, vector<string> >& actors_to_movies, ifstream& fin )
{
    string info, movie, actor;
    vector<string> actors, movies;

    //reads in the movie and all of its actors into a string
    while( getline( fin, info ) )
    {
	int m = 0;

	//reads until the first |, inputting everything before it into a string, this is the movie
	while( info[m] != '|' )
	    m++;
	movie = info.substr( 0, m );

	//reads through info, putting actors into a vector
        for( unsigned int i = movie.size() + 1; i < info.size(); i++ )
	{
	    int j = i;
	    while( info[i] != '|' && i < info.size() )
	        i++;

	    actor = info.substr( j, i-j );

	    actors.push_back( actor );

	    //if actor does not already have a key, creates a key and adds the movie to the element vector
	    if( actors_to_movies.find( actor ) == actors_to_movies.end() )
	    {
		movies.push_back(movie);
		actors_to_movies.insert( pair <string, vector<string> > ( actor, movies ) );
		movies.clear();
	    }
	    //if the actor does have a key, adds the movie to the element vector
	    else
		actors_to_movies[ actor ].push_back( movie );

	}

	//adds a movie key to the map, with the vector of actors as the element
	movies_to_actors.insert( pair <string, vector<string> > (movie, actors) );
	actors.clear();
    }

}


/********************************************************************/
// This function builds the minimum spanning tree using
// breadth first search using a queue.
//
// We initialize the queue by enqueing the start actor into 
// the queue. 
// Then, while the queue is not empty, we dequeue an actor from
// the queue and enqueue every other actor that has been in a movie
// with this actor. 
// For every actor that we dequeue, we store its parent, its bacon
// number, and the movie that it has in common with it's parent.
/********************************************************************/
void build_mst( map <string, node>& actor_info, map <string, vector<string> > & movies_to_actors, map <string, vector<string> > & actors_to_movies, map <string, int>& visited_movies, map <string, int>& visited_actors, string start_actor)
{
    node info;
    string this_movie, this_actor, actor;
    queue<string> q;

    info.parent_actor = "";
    info.movie = "";
    info.bacon_number = 0;

    //sets the starting actors info
    actor_info.insert( pair <string, node> (start_actor, info) );
    q.push(start_actor);
    visited_actors[start_actor] = 1;

    while ( !q.empty())
    {
	actor = q.front();
	q.pop();
	
	//reads through the movies of the actor
	for( int i = 0; i < (int)actors_to_movies[actor].size(); i++ )
	{
	    this_movie = actors_to_movies[actor][i];

	    //sets the movie to visited, or skips it if already visited
	    if (visited_movies[this_movie] == 1)
		continue;
	    else
	        visited_movies[this_movie] = 1;

	    //reads through the actors of each movie
	    for( int j = 0; j < (int)movies_to_actors[this_movie].size(); j++ )
	    {
		this_actor = movies_to_actors[this_movie][j];

		//sets the actor to visited, or skips it if already visited
 		if( visited_actors[this_actor] == 1 )
		    continue;
		else
		    visited_actors[this_actor] = 1;

		info.parent_actor = actor;
		info.movie = this_movie;
		info.bacon_number = actor_info[actor].bacon_number + 1;

		//sets the info of the actor
		actor_info.insert( pair <string, node> (this_actor, info) );
		q.push( this_actor );
	    }
	}
    }
}

/********************************************************************/
// This function outputs the statistics that were calculated.
/********************************************************************/
void output_info( map <int, int> & frequency, map <string, node> & actor_info, map <string, vector<string> > & actors_to_movies, int total_movies, int total_actors, clock_t t, clock_t s, string start_actor )
{
    int longest_path = 0, no_path, sum = 0, total_paths = 0;
    double avg_path;
    map <string, node> :: iterator itr;
    map <int, int> :: iterator itr2;

    cout << "Read... " << total_movies << " movies, counted " << total_actors << " actors in database (elapsed time " << (float)t / CLOCKS_PER_SEC << "s)" << endl;
    cout << "Found " << actors_to_movies[start_actor].size() << " " << start_actor << " movies" << endl;
    cout << "Building MST...done! (elapsed time: " << (float)s / CLOCKS_PER_SEC << "s)\n" << endl;

    cout << "Bacon number     frequency" << endl;

    //reads through the histogram and outputs the information
    for( itr2 = frequency.begin(); itr2 != frequency.end(); ++itr2 )
    {
	cout << setw(7) << itr2->first << setw(19) << itr2->second << endl;
	sum += itr2->first * itr2->second;
        total_paths += itr2->second;

	//sets the longest path to the largets bacon number
        longest_path = itr2->first;
    }

    //calculates the avg length of the paths
    avg_path = double(sum) / double(total_paths);
    //calculates the total number of actors that don't have a path to the starting actor
    no_path = total_actors - total_paths;

    cout << "no path" << setw(19) << no_path << endl << endl;
    cout << "total actors = " << total_actors << endl;
    cout << "total paths  = " << total_paths << endl;
    cout << "average path length = " << avg_path << endl << endl;

    cout << "Longest shortest paths: Bacon number = " << longest_path << endl;

    //displays the path of each actor with the largest bacon number to the starting actor
    for( itr = actor_info.begin(); itr != actor_info.end(); ++itr )
    {
	if( itr->second.bacon_number == longest_path )
	{
	    display_longest_path( itr->first, start_actor, actor_info );
	    cout << endl;
	}
    }
}

/********************************************************************/
// This function displays the path from actor to start.
/********************************************************************/
void display_longest_path (string actor, string start, map <string,node> &info)
{
    cout << actor;
    if( actor == start )
	return;
    cout << " -> ";

    //outputs the parent actor until it reaches the starting actor
    display_longest_path( info[actor].parent_actor, start, info );
}

/********************************************************************/
// This function diplays the path from actor to start
// This function is used when querying actors in the database.
/********************************************************************/
void display_path (string actor, string start, map <string,node> &info)
{
    cout << info[actor].bacon_number << ": " << actor << endl;
    if( info[actor].bacon_number == 0 )
	return;
    cout << "     " << info[actor].movie << endl;

    //displays the parent actor until bacon number reaches 0
    display_path( info[actor].parent_actor, start, info );
}
