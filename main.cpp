#include <iostream>
#include <typeinfo>
#include <cstdint>
#include <string>
#include <future>
#include <mutex>
#include <list>
#include <cctype>

int fromChar( char ch, int base, bool & aOK )
{
    if ( ch == '-' )
    {
        aOK = true;
        return 1;
    }
    aOK = false;
    if ( (ch >= '0') && ch <= ('0' + (base - 1)) )
    {
        aOK = true;
        return (ch - '0');
    }

    if ( base <= 10 )
        return 0;

    ch = std::tolower( ch );
    auto maxChar = 'a' + base;

    if ( (ch >= 'a') && (ch <= maxChar) )
    {
        aOK = true;
        return 10 + ch - 'a';
    }
    return 0;
}

char toChar( int value )
{
    if ( (value >= 0) && (value < 10) )
        return '0' + value;
    // over 10, must use chars

    return 'a' + value - 10;
}

std::string toString( int64_t val, int base )
{
    std::string retVal;
    do
    {
        int64_t quotient = val / base;
        int remainder = val % base;
        retVal.insert( retVal.begin(), toChar( remainder ) );
        val = quotient;
    } while( val != 0 );
    return retVal;
}

int64_t fromString( const std::string & str, int base )
{
    int64_t retVal = 0;
    bool isNeg = false;
    bool aOK = false;
    for ( size_t ii = 0; ii < str.length(); ++ii )
    {
        auto currChar = str[ ii ];
        if ( (ii == 0) && (currChar == '-') )
        {
            isNeg = true;
            continue;
        }

        int64_t currVal = (isNeg ? -1 : 1) * fromChar( currChar, base, aOK );
        if ( !aOK )
        {
            std::cerr << "Invalid character: " << currChar << std::endl;
            return 0;
        }
        retVal = (retVal * base) + currVal;
    }
    return retVal;
}

bool isNarcissistic( int64_t val, int base, bool & aOK )
{
    auto str = toString( val, base );

    int64_t sumOfPowers = 0;
    int64_t value = 0;
    bool isNeg = false;
    for ( size_t ii = 0; ii < str.length(); ++ii )
    {
        auto currChar = str[ ii ];
        if ( (ii == 0) && (currChar == '-') )
        {
            isNeg = true;
            continue;
        }

        int64_t currVal = (isNeg ? -1 : 1) * fromChar(currChar, base, aOK );
        if ( !aOK )
        {
            std::cerr << "Invalid character: " << currChar << std::endl;
            return false;
        }
        sumOfPowers += static_cast<uint64_t>(std::pow( currVal, str.length() ));

        value = (value * base) + currVal;
    }

    return value == sumOfPowers;
}

class CNarcissisticNumCalculator
{
public:
    bool parse( int argc, char ** argv )
    {
        for ( int ii = 1; ii < argc; ++ii )
        {
            bool aOK = false;
            if ( strncmp( argv[ ii ], "-base", 5 ) == 0 )
            {
                fBase = getInt( ii, argc, argv, "-base", aOK );
                if ( aOK && ((fBase < 2) || (fBase > 36)) )
                {
                    std::cerr << "Base must be between 2 and 36" << std::endl;
                    aOK = false;
                }
            }
            else if ( strncmp( argv[ ii ], "-max", 4 ) == 0 )
            {
                fMax = getInt( ii, argc, argv, "-max", aOK );
            }
            else if ( strncmp( argv[ ii ], "-thread_max", 11 ) == 0 )
            {
                fThreadMax = getInt( ii, argc, argv, "-max", aOK );
            }
            else if ( strncmp( argv[ ii ], "-numbers", 8 ) == 0 )
            {
                aOK = (ii + 1) < argc;
                if ( !aOK )
                {
                    std::cerr << "-numbers requires a list of integers\n";
                }
                while ( aOK )
                {
                    auto curr = getInt( ii, argc, argv, "-numbers", aOK );
                    if ( aOK )
                        fNumbers.push_back( curr );
                    if ( (ii + 1) >= argc )
                        break;
                }
            }

            if ( !aOK )
                return false;
        }
        return true;
    }

    void run()
    {
        report();
        createHandles();
        while ( !isFinished() )
            ;

        reportFindings();
    }
private:
    static int getInt( int & ii, int argc, char ** argv, const char * switchName, bool & aOK )
    {
        aOK = false;

        if ( ++ii == argc )
        {
            std::cerr << "-base requires a value";
            return 0;
        }
        const char * str = argv[ ii ];
        int retVal = 0;
        try
        {
            retVal = std::stoi( str );
            aOK = true;
        }
        catch ( std::invalid_argument const & e )
        {
            std::cerr << switchName << " value '" << str << "' is invalid. \n" << e.what() << "\n";
        }
        catch ( std::out_of_range const & e )
        {
            std::cerr << switchName << " value '" << str << "' is out of range. \n" << e.what() << "\n";
        }

        return retVal;
    }

    void dumpNumbers( const std::list< int64_t > & numbers ) const
    {
        bool first = true;
        size_t ii = 0;
        for ( auto && currVal : numbers )
        {
            if ( ii && (ii % 5 == 0) )
            {
                std::cout << "\n";
                first = true;
            }
            if ( !first )
                std::cout << ", ";
            else
                std::cout << "    ";
            first = false;
            std::cout << toString( currVal, fBase ) << "(=" << currVal << ")";
            ii++;
        }
        std::cout << "\n";
    }

    void reportFindings() const
    {
        std::cout << "There are " << fNarcissisticNumbers.size() << " Narcissistic numbers";
        if ( fNumbers.empty() )
            std::cout << " than or equal to " << fMax << "." << std::endl;
        else
            std::cout << " in the requested list." << std::endl;
        fNarcissisticNumbers.sort();
        dumpNumbers( fNarcissisticNumbers );
    }

    void report() const
    {
        if ( fNumbers.empty() )
        {
            std::cout << "Finding Narcissistic numbers up to: " << fMax << "\n";
            std::cout << "Maximum Numbers per thread: " << fThreadMax << "\n";
        }
        std::cout << "Base : " << fBase << "\n";
        if ( !fNumbers.empty() )
        {
            std::cout << "Checking if the following numbers are Narcissistic:\n";
            dumpNumbers( fNumbers );
        }
    }

    void findNarcissisticRange( int num, int base, int64_t min, int64_t max )
    {
        //std::cout << "\n" << num << ": Computing for (" << min << "," << max-1 << ")" << std::endl;
        int numArm = 0;
        for ( auto ii = min; ii < max; ++ii )
        {
            bool aOK = true;
            bool isNarcissistic = ::isNarcissistic( ii, base, aOK );
            if ( !aOK )
                return;
            if ( isNarcissistic )
            {
                //std::cout << curr << " is Narcissistic? " << (isNarcissistic ? "yes" : "no") << std::endl;
                addNarcissisticValue( ii );
                numArm++;
            }
        }
        //std::cout << "\n" << num << ": ----> Computing for (" << min << "," << max-1 << ")" << " = " << numArm << std::endl;
    }

    void findNarcissisticList( int base, const std::list< int64_t > & values )
    {
        int numArm = 0;
        for ( auto ii : values )
        {
            bool aOK = true;
            bool isNarcissistic = ::isNarcissistic( ii, base, aOK );
            if ( !aOK )
                return;
            if ( isNarcissistic )
            {
                //std::cout << curr << " is Narcissistic? " << (isNarcissistic ? "yes" : "no") << std::endl;
                addNarcissisticValue( ii );
                numArm++;
            }
        }
        //std::cout << "\n" << num << ": ----> Computing for (" << min << "," << max-1 << ")" << " = " << numArm << std::endl;
    }

    void createHandles()
    {
        //std::cout << "Creating Handles for for (" << min << "," << max - 1 << ")" << std::endl;
        if ( fNumbers.empty() )
        {
            int num = 0;
            for ( auto ii = 0; ii < fMax; ii += fThreadMax )
            {
                sHandles.push_back( std::async( std::launch::async, &CNarcissisticNumCalculator::findNarcissisticRange, this, num++, fBase, ii, ii + fThreadMax ) );
            }
        }
        else
            sHandles.push_back( std::async( std::launch::async, &CNarcissisticNumCalculator::findNarcissisticList, this, fBase, fNumbers ) );
    }

    bool isFinished()
    {
        while ( !sHandles.empty() )
        {
            for ( auto ii = sHandles.begin(); ii != sHandles.end(); )
            {
                if ( (*ii).wait_for( std::chrono::seconds( 0 ) ) == std::future_status::ready ) // finished
                    ii = sHandles.erase( ii );
                else
                    break; // still running
            }
        }
        return sHandles.empty();
    }

    void addNarcissisticValue( int64_t value )
    {
        std::lock_guard< std::mutex > lock( fMutex );
        fNarcissisticNumbers.push_back( value );
    }

    int fBase{ 10 };
    int fMax{ 100000 };
    int fThreadMax{ 100 };
    std::list< int64_t > fNumbers;
    std::list< std::future< void > > sHandles;

    std::mutex fMutex;
    mutable std::list< int64_t > fNarcissisticNumbers;
};

int main( int argc, char ** argv )
{
    CNarcissisticNumCalculator values;
    if ( !values.parse( argc, argv ) )
        return 1;

    values.run();
    return 0;
}

