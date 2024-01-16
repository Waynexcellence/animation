#include "type.h"
char density[96] = " .`_':-,;~/i^\\*!|vtrcs)l}1{?zj(Je+I7a]x[f<>yo3=LhuVb54TwknY2ACqUGFm9dZp6KgPXSEO0N8#&DRBH@WQM$";
int length[2] = {}; // [0]是strlen，[1]是256/[0]
BMP frames[8] = {}; // 使用 index 0~7 儲存bmp，*.bmp 從1開始
Load_Argument load_argument[9] = {};
Cut_Argument cut_argument[2] = {};


void exit_function(){
    fprintf( stderr , RESET );
    setvbuf(stdout, NULL, _IONBF, 0);
    fprintf( stderr , SHOW );
}
void Ctrl_C_handler(){
    fprintf( stderr , RESET );
    setvbuf(stdout, NULL, _IONBF, 0);
    fprintf( stderr , SHOW );
    exit(0);
}
void next_stage(){
    char temp[64];
    fprintf( stderr , GRN"Prese Enter to continue...\n"RESET );
    read( 0 , temp , sizeof(temp) );
    return;
}
void rdt_send( int send_fd , int rece_fd ){
    srand(time(NULL));
    char r = '0'+(rand()&7);
    char buf[256] = {};
    write( send_fd , &r , sizeof(char) );
    while(1){
        read( rece_fd , buf , sizeof(buf) );
        if( buf[0]==r ) break;
        else{
            perror("rdt_send");
            _exit(-1);
        }
        write( send_fd , &r , sizeof(int) );
    }
}
void rdt_receive( int send_fd , int rece_fd ){
    char buf[256] = {};
    while(1){
        read( rece_fd , buf , sizeof(buf) );
        if( strlen(buf)>1 ){
            fprintf( stderr , RED"\nrdt_receive strlen>1 [%s]\n"RESET , buf );
        }
        else if( buf[0]<'0' || buf[0]>'7' ){
            fprintf( stderr , RED"error rdt_receive\n"RESET );
            perror("rdt_receive");
            char e = '$';
            write( send_fd , &e , sizeof(char) );
            int dead = wait(&dead);
            break;
        }
        else break;
    }
    write( send_fd , &(buf[0]) , sizeof(char) );
}
void bmpLoad( BMP *bmp , char *filename ){
    BYTE header[HEADER_SIZE];
    BYTE info[256];
    FILE *file;
    if ((file = fopen(filename, "rb") ) == NULL ) {
        fprintf(stderr, RED"Error: bmpLoad(), %s open fail!\n"RESET , filename);
        perror("fopen");
        exit(0);
    }

    fread( header , HEADER_SIZE , 1 , file );
    fread( &(bmp->infosize) , 4 , 1 , file ); assert( bmp->infosize<256 );
    fseek( file , -4 , SEEK_CUR );
    fread( info , bmp->infosize , 1 , file );
    // Header
    bmp->signature       = B2U16(header,0);  assert(bmp->signature == 0x4D42);
    bmp->fileSize        = B2U32(header,2);
    bmp->dataOffset      = B2U32(header,10);
    // InfoHeader
    bmp->infosize        = B2U32(info,0);    assert(bmp->infosize+HEADER_SIZE==bmp->dataOffset);
    bmp->width           = B2U32(info,4);
    bmp->height          = B2U32(info,8);
    bmp->planes          = B2U16(info,12);   assert(bmp->planes==1);
    bmp->bitsPerPixel    = B2U16(info,14);   assert(bmp->bitsPerPixel==24);
    bmp->compression     = B2U32(info,16);
    bmp->imageSize       = B2U32(info,20);
    bmp->xPixelsPerM     = B2U32(info,24);
    bmp->yPixelsPerM     = B2U32(info,28);
    bmp->colorsUsed      = B2U32(info,32);
    bmp->colorsImportant = B2U32(info,36);
    // Data
    int padding = 4-((bmp->width*3)&3);
    fseek( file , bmp->dataOffset , SEEK_SET );
    bmp->data = (BYTE**) calloc( bmp->height , sizeof(BYTE*) );
    for(int x=0;x<bmp->height;x++){
        bmp->data[x] = (BYTE*) calloc( bmp->width , sizeof(Pixel) );
        fread( bmp->data[x] , bmp->width*3 , 1 , file );
        fseek( file , padding , SEEK_CUR );
    }
    fclose (file);
}
void bmpFree( BMP *bmp ){
    for(int x=0;x<bmp->height;x++){
        free(bmp->data[x]);
    }
    free(bmp->data);
}
void response_show_bmp( BMP *bmp ){
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    int width_mag = w.ws_col/bmp->width;
    int height_mag = w.ws_row/bmp->height;
    int min_mag = ( width_mag>height_mag )? height_mag : width_mag;
    if( min_mag==0 ){
        fprintf( stderr , RED"Please make your terminal bigger!!!"RESET );
        return;
    }
    for(int x=min_mag*bmp->height-1;x>=0;x--){
        for(int y=0;y<min_mag*(bmp->width);y++){
            Pixel *p = (Pixel*) &(bmp->data[x/min_mag][y/min_mag*3]);
            int average = (p->R+p->G+p->B)/3;
            fprintf( stderr , "%c" , (average>127)? '$':'.' );
        }
        fprintf( stderr , "\n" );
    }
    fprintf( stderr , "\n" );
}
void valid_input( int argc , char** argv ){                             // [1]檢查輸入格式
    if( argc!=2 ){
        fprintf( stderr , RED"\nformat: ./bmp [.mp4]\n"RESET );
        exit(-1);
    }
    FILE* fp = fopen( argv[1] , "r" );
    if( !fp ){
        fprintf( stderr , RED"\nfile doesn't exist\n"RESET );
        exit(-1);
    }
    else fclose( fp );
}
void create_dir( char* dir_name ){                                      // [2]創建資料夾
    int status = mkdir( dir_name , 0777 );
    if( status<0 && errno!=EEXIST ){
        fprintf( stderr , RED"\nmkdir errno = %d\n"RESET , errno );
        perror("mkdir");
        exit(-1);
    }
    errno = 0;
}
void get_info( const char* input , Info* info ){                        // [3]獲取影片資訊
    int pipe_fd[2] = {};
    pipe( pipe_fd );
    pid_t pid = fork();
    if( pid==0 ){
        close( pipe_fd[0] );
        dup2( pipe_fd[1] , STDOUT_FILENO );
        char command[512] = {};
        sprintf( command , "ffprobe -v error -select_streams v:0 -show_entries stream=r_frame_rate -of default=noprint_wrappers=1:nokey=1 %s" , input );
        system( command );
        sprintf( command , "ffprobe -v error -select_streams v:0 -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 %s" , input );
        system( command );
        sprintf( command , "ffprobe -v error -select_streams v:0 -show_entries stream=width,height -of csv=s=x:p=0 %s" , input );
        system( command );
        close( pipe_fd[1] );
        _exit(0);
    }
    else{
        char buf[256] = {};
        int a , b;
        close( pipe_fd[1] );
        read( pipe_fd[0] , buf , sizeof(buf) );
        sscanf( buf , "%d/%d" , &a , &b );
        info->frame_rate_a = a;
        info->frame_rate_b = b;
        read( pipe_fd[0] , buf , sizeof(buf) );
        sscanf( buf , "%d" , &a );
        info->period = a;
        read( pipe_fd[0] , buf , sizeof(buf) );
        sscanf( buf , "%dx%d" , &a , &b );
        info->width = a;
        info->height = b;
        close( pipe_fd[0] );
        int dead = wait( &dead );
    }
}
void buffer_set( char* buffer , int length ){                           // [4]緩衝區設置
    if( !buffer ){
        fprintf( stderr , RED"\n[Error] calloc return NULL.\n"RESET );
        perror("malloc");
        exit(0);
    }
    setvbuf( stdout , buffer , _IOFBF , length );
}
int available_screen( const Info info ){                                // [5]螢幕大小設置
    BMP bigger = {};
    bmpLoad( &bigger , "bigger.bmp" );
    int mag = (info.width>info.height)? info.width : info.height;
    mag = (mag>64)? mag>>6 : 1;
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    while( w.ws_col<70 || w.ws_row*mag<info.height ){
        response_show_bmp( &bigger );
        next_stage();
        ioctl(0, TIOCGWINSZ, &w);
        fprintf( stderr , RED"window size:\n\tcol = %d\n\trow = %d\n"RESET , w.ws_col , w.ws_row );
    }
    bmpFree( &bigger );
    return mag;
}
int cut_frame( const char* input , int start , int end , int which ){
    int pipe_fd[2][2] = {};
    pipe( pipe_fd[0] );
    pipe( pipe_fd[1] );
    pid_t pid = fork();
    if( pid==0 ){
        close( pipe_fd[0][0] );
        close( pipe_fd[1][1] );
        dup2( pipe_fd[1][0] , STDIN_FILENO );
        dup2( pipe_fd[0][1] , STDOUT_FILENO );
        char command[512] = {};
        sprintf( command , "rm -f ./Data%d/*.bmp" , which );
        system( command );
        sprintf( command , "ffmpeg -v error -i %s -ss %d -t %d -pix_fmt rgb24 ./Data%d/%%d.bmp" , input , start , end-start , which );
        system( command );
        sprintf( command , "ls ./Data%d/*.bmp | wc -l" , which );
        rdt_send( pipe_fd[0][1] , pipe_fd[1][0] );
        system( command );
        close( pipe_fd[0][1] );
        close( pipe_fd[1][0] );
        _exit(0);
    }
    else{
        char buf[256] = {};
        int num = 0;
        close( pipe_fd[1][0] );
        close( pipe_fd[0][1] );
        rdt_receive( pipe_fd[1][1] , pipe_fd[0][0] );
        read( pipe_fd[0][0] , buf , sizeof(buf) );
        sscanf( buf , "%d" , &num );
        int dead = wait( &dead );
        close( pipe_fd[1][1] );
        close( pipe_fd[0][0] );
        return num;
    }
}
void max_color_show_bmp( BMP *bmp , int mag ){
    int height_bound = bmp->height/mag;
    int width_bound = bmp->width/mag;
    char map[width_bound];
    for(int x=height_bound-1;x>=0;x--){
        for(int y=0;y<width_bound;y++){
            Pixel *p = (Pixel*) &(bmp->data[x*mag][y*mag*3]);
            int average = (p->G+p->R+p->B)/3;
            int index = average/length[1];
            index = (index<0)? 0 : (index>=length[0])? length[0]-1 : index;
            map[y] = density[index];
        }
        fprintf( stdout , "%s\n" , map );
    }
    printf( CORNER );   // Move cursor to the top-left corner
    fflush( stdout );
}
void* thread_cut( void* arg ){
    Cut_Argument* pointer = (Cut_Argument*) arg;
    pointer->result = cut_frame( pointer->input , pointer->start , pointer->end , pointer->which );
    pthread_exit(NULL);
}
void* thread_load( void* arg ){
    Load_Argument* pointer = (Load_Argument*) arg;
    char filename[32] = {};
    sprintf( filename , "./Data%d/%d.bmp" , pointer->source , pointer->target );
    bmpLoad( &frames[(pointer->target-1)&7] , filename );
    pthread_exit(NULL);
}
int main( int argc , char** argv ) {
    atexit( &exit_function );
    signal( SIGINT , Ctrl_C_handler );
    length[0] = strlen(density);
    length[1] = 256/length[0];
    // 會用到的區域變數
    Info info = {};         // 給[3]，讀取影片資料
    char* stdout_buffer;    // 給[5]，輸出緩衝區設置
    pthread_t thread[9];
    // 程式開始
    fprintf( stderr , YEL"[Processd] check the input format"RESET );
    valid_input( argc , argv );
    fprintf( stderr , DELETE""GRN"[Finished] Correct input format\n"RESET );
    // [1]格式檢查完成
    fprintf( stderr , YEL"[Processd] mkdir directory [Data]"RESET );
    create_dir( "./Data0" );
    create_dir( "./Data1" );
    fprintf( stderr , DELETE""GRN"[Finished] set the data directory\n"RESET );
    // [2]創造資料夾完成
    fprintf( stderr , YEL"[Processd] get the info from [%s]"RESET , argv[1] );
    get_info( argv[1] , &info );
    fprintf( stderr , DELETE""GRN"[Finished] get the information from [%s]\n"RESET , argv[1] );
    fprintf( stderr , RED"MP4 file information:\n"RESET );
    fprintf( stderr , RED"\tframe-rate   :%4d/%4d\n"RESET , info.frame_rate_a , info.frame_rate_b  );
    fprintf( stderr , RED"\twidth        :%4d    \n"RESET , info.width  );
    fprintf( stderr , RED"\theight       :%4d    \n"RESET , info.height );
    fprintf( stderr , RED"\tperiod       :%4d    \n"RESET , info.period );
    if( info.frame_rate_a/info.frame_rate_b >= 50 ){
        fprintf( stderr , RED"Only support frame rate <= 49\n"RESET );
        exit(0);
    }
    info.period &= ((-1)<<1);
    fprintf( stderr , RED"I will display the %ds of the mp4 file.\n"RESET , info.period );
    // [3]讀取影片資料完成
    fprintf( stderr , YEL"[Processd] setting the stdout"RESET );
    stdout_buffer = (char*) malloc( 10000 );
    buffer_set( stdout_buffer , 10000 );
    fprintf( stderr , DELETE""GRN"[Finished] set the stdout\n"RESET );
    // [4]輸出緩衝區配置完成
    int mag = available_screen( info );
    // int mag = 10;
    fprintf( stderr , GRN"[Finished] set the window size available to see the mp4\n"RESET );
    // [5]螢幕大小配置完成
    next_stage();
    strcpy( cut_argument[0].input , argv[1] );
    strcpy( cut_argument[1].input , argv[1] );
    cut_argument[0].start = 0;
    cut_argument[0].end = 2;
    cut_argument[0].which = 0;
    pthread_create( &thread[8] , NULL , thread_cut , &cut_argument[0] );
    fprintf( stderr , RED"cut 0 - 2 to ./Data0\n"RESET );
    for(int x=0;x<info.period;x+=2){    // 2秒一次循環
        int which = (x>>1)&1;               // 0代表存放在Data0，1代表存放在Data1
        pthread_join( thread[8] , NULL );
        if( cut_argument[which].result<=8 ){        // 切割的frame少於8幀
            fprintf( stderr , RED"[Error] frame<=8\n"RESET );
            break;
        }
        for(int y=0;y<8;y++){
            load_argument[y].source = which;
            load_argument[y].target = y+1;
            pthread_create( &thread[y] , NULL , thread_load , (void*)&load_argument[y] );
        }
        for(int y=0;y<cut_argument[which].result;y++){
            pthread_join( thread[(y&7)] , NULL );
            max_color_show_bmp( &frames[(y&7)] , mag );
            usleep( 100000 );
            bmpFree( &frames[(y&7)] );
            if( y+1+8<=cut_argument[which].result ){
                load_argument[(y&7)].target = y+1+8;
                pthread_create( &thread[(y&7)] , NULL , thread_load , (void*)&load_argument[(y&7)] );
            }
            // fprintf( stderr , YEL"show %d/%d.bmp\n"RESET , which , y+1 );
            if( y==((cut_argument[which].result)>>1) ){
                int another = (which)? 0 : 1;
                cut_argument[another].start = x+2;
                cut_argument[another].end = x+4;
                cut_argument[another].which = another;
                pthread_create( &thread[8] , NULL , thread_cut , &cut_argument[another] );
            }
        }
    }
    return 0;
}