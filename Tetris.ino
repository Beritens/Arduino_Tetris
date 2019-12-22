    /*
          8x8 LED Matrix MAX7219 Scrolling Text Example
       Based on the following library:
       GitHub | riyas-org/max7219  https://github.com/riyas-org/max7219
    */
    #include <MaxMatrix.h>
    #include <LiquidCrystal_I2C.h>
    #include <avr/pgmspace.h>

     LiquidCrystal_I2C lcd(0x27,16,2);

     byte oben[] = {
        B11111,
        B11111,
        B11111,
        B11111,
        B11111,
        B00000,
        B00000,
        B00000
      };


     byte unten[] = {
        B00000,
        B00000,
        B00000,
        B11111,
        B11111,
        B11111,
        B11111,
        B11111
      };
      
    
    const int SW_pin = 2;
    const int X_pin = 0;
    const int Y_pin =1;

  
    int DIN = 7;   // DIN pin of MAX7219 module
    int CLK = 6;   // CLK pin of MAX7219 module
    int CS = 5;    // CS pin of MAX7219 module
    int maxInUse = 2;
    MaxMatrix m(DIN, CS, CLK, maxInUse);
    //byte buffer[10];
    int points[8][16];
    int pos[2] = {0,0};
    int piece = 0;
    int bag[7] = {0,1,2,3,4,5,6};
    int nextBag[7] = {0,1,2,3,4,5,6};
    int rotation = 0;

    int inputX = 0;
    int inputY = 0;
    
    bool dotActive = false;
    int t = 0;
    int controlDelay = 1;
    int rotateDelay = 1;
    int fastRotate = 0;
    int fastControl = 0;
    int startRotate = 0;
    int startControl = 0;
                          // I-block               O-block              J-block                  L-block                S-block                 Z-block                T-block
    int blocks[7][3][2] = {{{-1,0},{1,0},{2,0}},  {{0,1},{1,1},{1,0}},  {{-1,1},{-1,0},{1,0}},  {{-1,0},{1,0},{1,1}},  {{-1,0},{0,1},{1,1}},  {{-1,1},{0,1},{1,0}},  {{-1,0},{0,1},{1,0}}};

    int offsetDataJLSTZ[5][4][2] = {{{0,0},{0,0},{0,0},{0,0}},{{0,0},{1,0},{0,0},{-1,0}},{{0,0},{1,-1},{0,0},{-1,-1}},{{0,0},{0,2},{0,0},{0,2}},{{0,0},{1,2},{0,0},{-1,2}}};
    int offsetDataI[5][4][2] = {{{0,0},{-1,0},{-1,1},{0,1}},{{-1,0},{0,0},{1,1},{0,1}},{{2,0},{0,0},{-2,1},{0,1}},{{-1,0},{0,1},{1,0},{0,-1}},{{2,0},{0,-2},{-2,0},{0,-2}}};
    int offsetDataO[4][2] = {{0,0},{0,-1},{-1,-1},{-1,0}};


    //score stuff
    int score = 0;
    int lines = 0;
    int level = 1;

    int inGame = 0;
    
    
    void setup() {
      
    
      //joy-stick
      pinMode(SW_pin,INPUT);
      digitalWrite(SW_pin,HIGH);
      Serial.begin(9600);

      lcd.init();
      lcd.backlight();
      lcd.createChar(0,oben);
      lcd.createChar(1,unten);
      
      m.init(); // module initialize
      m.setIntensity(15); // dot matix intensity 0-15
      for(int x = 0; x<8; x++){
        for(int y = 0; y<16; y++){
          points[x][y]= 0;
          m.setDot(y,x,0);
        }
      }
      scoring(0);
    }
    void loop() {
      if(digitalRead(SW_pin)==0){
        reset();
      }
      if(inGame!= 0){
        if(inGame==1){
          GameOver();
        }
        return;
      }
       inputX = analogRead(X_pin);
       inputY = analogRead(Y_pin);
       
       t++;

       if(t% controlDelay ==startControl && dotActive){
          control();
       }
       if(t% rotateDelay ==startRotate && dotActive){
          rotate();
       }
       int fallSpeed = max(80-10*level,20);
       if(inputY>900){
        fallSpeed = 5;
       }
       if(t%fallSpeed == 0){
          if(!dotActive){
            Spawn();
         }
         moveDown();
       }
       
       delay(10);
    }
    void reset(){
       t=0;
       inGame = 0;
       level = 1;
       score = 0;
       lines = 0;
       piece = -1;
       shuffleBag(nextBag);
       displayPiece(nextPiece(7));
       dotActive = false;
       for(int x = 0; x<8; x++){
        for(int y = 0; y<16; y++){
          points[x][y]= 0;
        }
      }
      StartGame();
      scoring(0);
       
    }
    void StartGame(){
      for(int i = 0; i<23; i++){
        for(int j = max(i-15,0); j< i+1; j++){
          m.setDot(j,i-j,0);
        }
        delay(100);
      }
    }
    void GameOver(){
      for(int i = 0; i<23; i++){
        for(int j = max(i-15,0); j< i+1; j++){
          m.setDot(j,i-j,1);
        }
        delay(100);
      }
      inGame= 2;
    }
    void rotate(){
      
      if(inputY <150){
        
        fastRotate++;
        rotateDelay = 20;
        if(fastRotate >= 4){
          rotateDelay = 3;
        }
        startRotate = t%rotateDelay;
        bool yes = false;
        int xof = 0;
        int yof = 0;
        if(bag[piece] == 0){
          for(int i = 0; i<5; i++){
            int ofx = offsetDataI[i][rotation][0]-offsetDataI[i][(rotation+1)%4][0];
            int ofy = offsetDataI[i][rotation][1]-offsetDataI[i][(rotation+1)%4][1];
            if(testPiece(pos[0]+ofx,pos[1]+ofy,(rotation+1)%4)){
              yes = true;
              xof = ofx;
              yof = ofy;
              break;
            }
          }
        }
        else if(bag[piece] == 1){
          int ofx = offsetDataO[rotation][0]-offsetDataO[(rotation+1)%4][0];
          int ofy = offsetDataO[rotation][1]-offsetDataO[(rotation+1)%4][1];
          if(testPiece(pos[0]+ofx,pos[1]+ofy,(rotation+1)%4)){
            yes = true;
            xof = ofx;
            yof = ofy;
          }
        }
        else{
          for(int i = 0; i<5; i++){
            int ofx = offsetDataJLSTZ[i][rotation][0]-offsetDataJLSTZ[i][(rotation+1)%4][0];
            int ofy = offsetDataJLSTZ[i][rotation][1]-offsetDataJLSTZ[i][(rotation+1)%4][1];
            if(testPiece(pos[0]+ofx,pos[1]+ofy,(rotation+1)%4)){
              yes = true;
              xof = ofx;
              yof = ofy;
              break;
            }
          }
        }
        if(yes){
          
          printPiece(pos[0],pos[1],0,false,rotation);
          rotation  = (rotation +1)%4;
          pos[0] += xof;
          pos[1] += yof;
          printPiece(pos[0],pos[1],1,false,rotation);
        }
      }
      else{
        rotateDelay = 1;
        fastRotate = 0;
        startRotate = t%rotateDelay;
      }
    }
    void control(){
      int dir = 0;
      if(abs(inputX-512)>250){
        fastControl++;
        controlDelay = 20;
        if(fastControl >= 4){
          controlDelay = 1;
        }
        startControl = t%controlDelay;
      
        
        if(inputX-512>0){
          dir = 1;
        }
        else{
          dir = -1;
        }
        
        if(testPiece(pos[0]+dir,pos[1],rotation)){
          printPiece(pos[0],pos[1],0,false,rotation);
          pos[0] += dir;
          printPiece(pos[0],pos[1],1,false,rotation);
        }
      }
      else{
        fastControl = 0;
        controlDelay = 1;
        startControl = t%controlDelay;
      }
    }
    void Spawn(){
      piece++;
      if(piece%7 == 0){
        piece = 0;
        for(int i = 0; i<7; i++){
          bag[i] = nextBag[i];
        }
        shuffleBag(nextBag);
        
      }
      rotation = 0;
      pos[0] = 3;
      pos[1] = 15;
      displayPiece(nextPiece(piece+1));
      dotActive = true;
      printPiece(pos[0],pos[1],1,false,rotation);
    }
    void shuffleBag(int bagy[7]){
      for (int i = 0; i < 7; i++) {    // shuffle array
          int temp = bagy[i];
          int randomIndex = rand() % 7;
      
          bagy[i]           = bagy[randomIndex];
          bagy[randomIndex] = temp;
      }
    }
    int nextPiece(int next){
       if(next <7){
        return bag[next];
       }
       else{
        return nextBag[next-7];
       }
    }
    void displayPiece(int _piece){
      lcd.setCursor(12,0);
      lcd.print("    ");
      lcd.setCursor(12,1);
      lcd.print("    ");
      lcd.setCursor(13,1);
      lcd.write(0);
      for(int i = 0; i< 3; i++){
        lcd.setCursor(13+blocks[_piece][i][0],1-blocks[_piece][i][1]);
        lcd.write(blocks[_piece][i][1]);
      }
    }
    
    void moveDown(){
      bool  valid = testPiece(pos[0],pos[1]-1,rotation);
      
      
      
      if(valid){
        printPiece(pos[0],pos[1],0,false,rotation);
        pos[1] --;
        printPiece(pos[0],pos[1],1,false,rotation);
      }
      else{
        land();
        
      }
    }
    void scoring(int rows){
      lines += rows;
        int mul = 0;
      switch(rows){
          case 0: 
            mul = 0;
            break;
          case 1: 
            mul = 40;
            break;
          case 2: 
            mul = 100;
            break;
          case 3: 
            mul = 300;
            break;
          case 4: 
            mul = 1200;
            break;
          
        }
        score += mul*(level);
        level = lines/10+1;
        lcd.setCursor(0,0);
        lcd.print("            ");
        lcd.setCursor(0,0);
        lcd.print(level);
        lcd.print(", ");
        lcd.print(lines);
        lcd.setCursor(0,1);
        lcd.print("            ");
        lcd.setCursor(0,1);
        lcd.print("scr:");
        lcd.print(score);
    }
    void land(){
      printPiece(pos[0],pos[1],1,true,rotation);
      score += 16;
        dotActive = false;
        int rows[4]= {-1,-1,-1,-1};
        int rowCount = 0; 
        if(testRow(pos[1])){
          rows[rowCount] = pos[1];
          rowCount++;
        }
        for(int i = 0; i<3; i++){
          int y = GetBlockPos(pos[1],i,rotation,1);
          if(testRow(y)){
            bool ok = true;
            for(int j = 0; j<4 ; j++){
              if(rows[j]==y){
                ok = false;
              }
            }
            if(ok){
              rows[rowCount] = y;
              rowCount++;
            }
          }
          
        }
        
        scoring(rowCount);
        
        if(rowCount>0){
          Serial.print("rowCount: ");
          Serial.print(rowCount);
          for(int i = 0; i<rowCount; i++){
            for(int j = 0; j<8; j++){
              m.setDot(rows[i],j,0);
            }
          }
          delay(100);
          for(int y = 0; y<16; y++){
            int a = 0;
            bool aRow = false;
            for(int i = 0; i<rowCount; i++){
              if(y == rows[i]){
                aRow = true;
              }
              if(y> rows[i]){
                a++;
              }
            }
            for(int x = 0; x<8; x++){
              if(!aRow){
                
                points[x][y-a] = points[x][y];
              }
              m.setDot(y-a,x,points[x][y-a]);
            }
          }
        }
        
        
    }
    void SetBlock(int x, int y, int a){
      if(y>= 15){
        inGame = 1;
      }
      points[x][y] = a;
      }
    bool testPiece(int x, int y, int r){
      if(!testDot(x,y)){
        return false;
      }
       for(int i = 0; i<3; i++){
          if(!testDot(GetBlockPos(x,i,r,0),GetBlockPos(y,i,r,1))){
            return false;
          }
       }
      return true;
    }
    bool testDot(int x, int y){
      if(x<0 || x>7 || y<0){
        return false;
      }
      else{
        return points[x][y] == 0;
      }
    }
    bool testRow(int row){
      bool yes = false;
      for(int i = 0; i<8; i++){
        if(points[i][row] == false){
          return false;
        }
      }
      return true;
    }
    void printPiece(int x, int y, int a, bool setBlock, int r){
      m.setDot(y,x,a);
      if(setBlock){
        SetBlock(x,y,a);
      }
      for(int i = 0; i<3; i++){
        m.setDot(GetBlockPos(y,i,r,1),GetBlockPos(x,i,r,0),a);
        if(setBlock){
           SetBlock(GetBlockPos(x,i,r,0),GetBlockPos(y,i,r,1),a);
        }
        
      }
    }
    int GetBlockPos(int pos, int i, int r, int xy){
      int j = xy;
      int m = 1;
      if(r == 1 || r==3){
        j = (j+1)%2;
      }
      switch(r){
        case 1:
          if(j== 0){
            m=-1;
          }
          break;
        case 2:
          m=-1;
          break;
        case 3:
          if(j== 1){
            m=-1;
          }
          break;
        default:
          m=1;
          break;
      }
      return pos+blocks[bag[piece]][i][j]*m;
    }
    
 
