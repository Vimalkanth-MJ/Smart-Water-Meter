const express = require('express');
const session = require('express-session');
const passport = require('passport');
const GoogleStrategy = require('passport-google-oauth20').Strategy;
const app = express();
const axios = require('axios');

const firebase = require('firebase/app');
require('firebase/database');


var firebaseConfig = {
  apiKey: "AIzaSyAXbbSPYCqxFcp9Ynb6nlmkkWcLQ8GaUgw",
  authDomain: "rechargeapp-watermeter.firebaseapp.com",
  projectId: "rechargeapp-watermeter",
  storageBucket: "rechargeapp-watermeter.appspot.com",
  messagingSenderId: "584772629044",
  appId: "1:584772629044:web:0cc9f50dd5d63de6869061",
  measurementId: "G-HHVL9BEDSD",
  databaseURL: "https://rechargeapp-watermeter-default-rtdb.asia-southeast1.firebasedatabase.app"
  };
  
  firebase.initializeApp(firebaseConfig);
// Set up session middleware

app.use(express.json());

app.use(session({
  secret: 'your-secret-key',
  resave: false,
  saveUninitialized: false
}));

// Initialize passport and session
app.use(passport.initialize());
app.use(passport.session());

// Configure Google strategy
passport.use(new GoogleStrategy({
  clientID: '584772629044-bv1i5igbfoi99rajde4rqu07udvlq78o.apps.googleusercontent.com',
  clientSecret: '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',
  callbackURL: 'http://localhost:3000/auth/google/callback',
  scope: ['profile', 'email']
},
(accessToken, refreshToken, profile, done) => {
  // Check if the user's email is available
  if (profile._json && profile._json.email) {
    // Extract the email from the user's profile
    const email = profile._json.email;
    
    // Store the email and ID in the session
    profile.email = email;
    profile.id = profile.id; // Store the Google ID
    
    // You can also store other user identification data from the profile if needed
    profile.name = profile.displayName; // Store the display name
    profile.picture = profile._json.picture; // Store the profile picture URL
  } else {
    // Handle the case when the email is not available
    console.log('Email not found in the user profile.');
    profile.email = null; // Set the email to null or handle it as needed
  }

  done(null, profile);
}));

// Serialize user into the session
passport.serializeUser((user, done) => {
  done(null, user);
});

// Deserialize user from the session
passport.deserializeUser((user, done) => {
  done(null, user);
});

// Define login route
app.get('/', passport.authenticate('google'));

// Define callback route for Google Sign-In
app.get('/auth/google/callback', passport.authenticate('google', { failureRedirect: '/login' }),
  (req, res) => {
    // Check if the user is authenticated
    if (req.isAuthenticated()) {
      // Redirect to index.html
      res.redirect('/index.html');
    } else {
      // Redirect back to the login page
      res.redirect('/');
    }
  }
);

// Define index route
app.get('/index.html', (req, res) => {
  // Check if the user is authenticated
  if (req.isAuthenticated()) {
    // Render the index.html page
    res.sendFile(__dirname + '/index.html');
  } else {
    // User is not authenticated, redirect back to the login page
    res.redirect('/');
  }
});

app.get('/index2.html', (req, res) => {
  // Check if the user is authenticated
  if (req.isAuthenticated()) {
    // Render the index.html page
    res.sendFile(__dirname + '/index2.html');
  } else {
    // User is not authenticated, redirect back to the login page
    res.redirect('/');
  }
});

// Define endpoint to get device code
app.get('/getDeviceCode', (req, res) => {
  // Retrieve the user's Google ID from the session
  const googleID = req.isAuthenticated() ? req.user.id : null;
  
  if (googleID) {
    // Retrieve device code from Firebase database
    var database = firebase.database();
    var ref = database.ref(googleID + '/' + googleID);
    
    ref.once('value')
      .then(snapshot => {
        var retrievedDeviceCode = snapshot.val();
        
        // Send the retrieved device code as a response
        res.send(retrievedDeviceCode);
      })
      .catch(error => {
        console.error('Error retrieving device code:', error);
        res.status(500).send('Error retrieving device code.');
      });
  } else {
    res.status(401).send('User not authenticated.');
  }
});


// Define endpoint to store device code
app.post('/storeDeviceCode', (req, res) => {
  const deviceCode = req.body.deviceCode;
  
  // Retrieve the user's Google ID from the session
  const googleID = req.isAuthenticated() ? req.user.id : null;
  
  if (googleID) {
    // Store device code in Firebase database
    var database = firebase.database();
    var ref = database.ref(googleID);

    ref.update({ [googleID]: deviceCode })
      .then(function() {
        res.status(200).send('Device code stored successfully!');
      })
      .catch(function(error) {
        console.log("Error storing data: " + error);
        res.status(500).send('Error storing device code.');
      });
  } else {
    res.status(401).send('User not authenticated.');
  }
});

// Define endpoint to get credits data
app.get('/getCreditsData', async (req, res) => {
  try {
    // Retrieve the user's Google ID from the session
    const googleID = req.isAuthenticated() ? req.user.id : null;

    if (googleID) {
      // Retrieve device code from Firebase database
      var database = firebase.database();
      var ref = database.ref(googleID + '/' + googleID);

      ref.once('value')
        .then(async snapshot => {
          var deviceCode = snapshot.val();

          // Make GET request to Blynk API
          const response = await axios.get(`https://blynk.cloud/external/api/get?token=${deviceCode}&v1`);
          const creditsData = response.data;

          // Send the retrieved credits data as a response
          res.send(creditsData);
        })
        .catch(error => {
          console.error('Error retrieving device code:', error);
          res.status(500).send('Error retrieving device code.');
        });
    } else {
      res.status(401).send('User not authenticated.');
    }
  } catch (error) {
    console.error('Error retrieving credits data:', error);
    res.status(500).send('Error retrieving credits data.');
  }
});


// Start the server
app.listen(3000, () => {
  console.log('Server is running on http://localhost:3000');
});
