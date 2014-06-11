package ws.jeroen.brucecontrol;

import ws.jeroen.brucecontrol.util.SystemUiHider;

import android.annotation.TargetApi;
import android.app.Activity;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.StrictMode;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;

import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.StatusLine;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.DefaultHttpClient;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

/**
 * An example full-screen activity that shows and hides the system UI (i.e.
 * status bar and navigation/system bar) with user interaction.
 *
 * @see SystemUiHider
 */
public class BruceControlMainActivity extends Activity {

    private final int minThrottle = -100;
    private final int maxThrottle = 100;

    private final int forwardThrottle = 100;
    private final int backwardThrottle = -100;
    private final int stopThrottle = 0;

    private final int minTilt = 0;
    private final int maxTilt = 100;

    private final int upTilt = 0;
    private final int straightTilt = 40;
    private final int downTilt = 95;

    enum eDirection { straight, left, right };

    class ThrottleTouchListener implements View.OnTouchListener
    {
        public ThrottleTouchListener(int aThrottle, eDirection aDirection)
        {
            theDirection = aDirection;
            theThrottle = aThrottle;
        }

        private eDirection theDirection;
        private int theThrottle;

        @Override
        public boolean onTouch( View button , MotionEvent aMotion)
        {
            switch ( aMotion.getAction() ) {
                case MotionEvent.ACTION_DOWN:
                    doThrottle(theThrottle, theDirection);
                    break;
                case MotionEvent.ACTION_UP:
                    doThrottle(stopThrottle, eDirection.straight);
                    break;
            }
            return true;
        }

        private void doThrottle(int aThrottleVal, eDirection aDirection){
            // Clip to acceptable boundaries
            aThrottleVal = Math.min(maxThrottle, aThrottleVal);
            aThrottleVal = Math.max(minThrottle, aThrottleVal);

            int leftThrottle = 0;
            int rightThrottle = 0;
            switch(aDirection) {
                case straight:
                    leftThrottle = aThrottleVal;
                    rightThrottle = aThrottleVal;
                    break;
                case left:
                    leftThrottle = aThrottleVal / 2;
                    rightThrottle = -aThrottleVal;
                    break;
                case right:
                    leftThrottle = -aThrottleVal;
                    rightThrottle = aThrottleVal / 2;
                    break;
            }

            new RequestTask().execute(String.format("http://192.168.1.2/1/pins/lprop?value=%d", leftThrottle));
            new RequestTask().execute(String.format("http://192.168.1.2/1/pins/rprop?value=%d", rightThrottle));
        }
    }

    class TiltTouchListener implements View.OnTouchListener
    {
        public TiltTouchListener(int aTile)
        {
            theTilt = aTile;
        }

        private int theTilt;

        @Override
        public boolean onTouch(View button , MotionEvent aMotion)
        {
            switch ( aMotion.getAction() ) {
                case MotionEvent.ACTION_DOWN:
                    doTilt(theTilt);
                    break;
                case MotionEvent.ACTION_UP:
                    doTilt(straightTilt);
                    break;
            }
            return true;
        }

        private void doTilt(int aTiltVal){
            // Clip to acceptable boundaries
            aTiltVal = Math.min(maxTilt, aTiltVal);
            aTiltVal = Math.max(minTilt, aTiltVal);

            new RequestTask().execute(String.format("http://192.168.1.2/1/pins/servo?value=%d", aTiltVal));
        }
    }

    class RequestTask extends AsyncTask<String, String, String> {

        @Override
        protected String doInBackground(String... uri) {
            HttpClient httpclient = new DefaultHttpClient();
            HttpResponse response;
            String responseString = null;
            try {
                response = httpclient.execute(new HttpGet(uri[0]));
                StatusLine statusLine = response.getStatusLine();
                if(statusLine.getStatusCode() == HttpStatus.SC_OK){
                    ByteArrayOutputStream out = new ByteArrayOutputStream();
                    response.getEntity().writeTo(out);
                    out.close();
                    responseString = out.toString();
                } else{
                    //Closes the connection.
                    response.getEntity().getContent().close();
                    throw new IOException(statusLine.getReasonPhrase());
                }
            } catch (ClientProtocolException e) {
                //TODO Handle problems..
            } catch (IOException e) {
                //TODO Handle problems..
            }
            return responseString;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_bruce_control_main);

        // Forward button
        Button b = (Button)findViewById(R.id.forwardButton);
        b.setOnTouchListener(new ThrottleTouchListener(forwardThrottle, eDirection.straight));

        // Turn right button
        b = (Button)findViewById(R.id.turnRightButton);
        b.setOnTouchListener(new ThrottleTouchListener(forwardThrottle, eDirection.right));

        // Turn left button
        b = (Button)findViewById(R.id.turnLeftButton);
        b.setOnTouchListener(new ThrottleTouchListener(forwardThrottle, eDirection.left));

        // Backward button
        b = (Button)findViewById(R.id.backwardButton);
        b.setOnTouchListener(new ThrottleTouchListener(backwardThrottle, eDirection.straight));

        // Up button
        b = (Button)findViewById(R.id.upButton);
        b.setOnTouchListener(new TiltTouchListener(upTilt));

        // Down button
        b = (Button)findViewById(R.id.downButton);
        b.setOnTouchListener(new TiltTouchListener(downTilt));
    }
}
